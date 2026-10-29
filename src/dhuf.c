/* DHUF: C with a 4096-byte LZ dictionary and adaptive Huffman decoder.
 * All extents are recovered from control flow, not the prologue census.
 * Far arrays/scalars deliberately retain the original 1E81 frame.
 */
#include <dos.h>
/* This TU addresses FILE objects through a far segment constant. Hide the
 * unused near declaration, then declare the same stock RTL array as far. */
#define _streams near_stream_declaration
#include <stdio.h>
#undef _streams
extern FILE far _streams[];
#include <stdlib.h>
#include <string.h>
#include "dzconst.h"
#define WINDOW 4096
#define LOOKAHEAD 60
#define THRESHOLD 2
#define SYMBOLS (256-THRESHOLD+LOOKAHEAD)
#define TREE (SYMBOLS*2-1)
#define ROOT (TREE-1)

extern int far exit_requested;


extern unsigned get_buffer, put_buffer;
extern unsigned char get_count, put_count;
extern unsigned bit_mask[17], bit_shift[17];
extern unsigned char position_code[256], position_length[256];
extern char *manual_filename;
extern char fatal_banner[], fatal_format[], progress_format[], complete_format[];
extern char resource_error[], read_mode[], write_mode[], open_error[], seek_error[];
extern char length_error[], extract_message[], complete_message[], close_error[];
extern char far executable_path[];
extern int far insecure_argument_index;
extern unsigned far image_data_offset;
extern long far resource_base;
void print_order_info(int);
void check_executable_integrity(void);
void read_mz_resource_base(FILE *);


/* Shared far workspace, physically declared in asm/seg1899.asm. */
extern FILE * far lzh_input;
extern FILE * far lzh_output;
extern long far expanded_size;
extern long far compressed_size;
extern long far progress_next;
extern long far progress_step;
extern unsigned char far dictionary[WINDOW+LOOKAHEAD-1];
extern unsigned far match_position;
extern unsigned far match_length;
extern int far left_child[WINDOW+1];
extern int far parent_node[WINDOW+1];
extern int far right_child[WINDOW*2+1];
extern unsigned char far prefix_length[WINDOW+1];
extern unsigned far huff_frequency[TREE+1];
extern int far huff_parent[TREE+SYMBOLS];
extern int far huff_son[TREE];

#define extraction_stdout (_streams[1])
#define extraction_stderr (_streams[2])

/* Returns the DHUF code segment.
 * Original 1A72:0007, 3 bytes. */
unsigned dhuf_seg(void) { return _CS; }

/* Prints a manual-extraction error and reason, then exits with status 500.
 * Original 1A72:000A, 84 bytes. */
void lzh_fatal(char *reason)
{
    exit_requested=1;
    fprintf(&extraction_stderr,fatal_banner);
    fprintf(&extraction_stderr,fatal_format,reason);
    print_order_info(0);
    exit(500);
}

/* Initializes unused right-child and parent entries to the dictionary sentinel.
 * Original 1A72:005E, 100 bytes. */
void lzh_init_dictionary_tree(void)
{
    int far *tree_node,*last_tree_entry;
    tree_node=&right_child[WINDOW+1]; last_tree_entry=&right_child[WINDOW*2];
    while(tree_node<=last_tree_entry) *tree_node++=WINDOW;
    tree_node=parent_node; last_tree_entry=&parent_node[WINDOW];
    while(tree_node<last_tree_entry) *tree_node++=WINDOW;
}

/* Inserts a dictionary position into the binary search tree and updates the longest match.
 * Original 1A72:00C2, 742 bytes. */
void lzh_insert_node(int r)
{
    int tree_node,comparison;
    unsigned char *match_key;
    unsigned candidate_distance,common_prefix_length,parent_prefix_length;
    comparison=1; match_key=&dictionary[r];
    common_prefix_length=match_key[1]^match_key[2]; common_prefix_length^=common_prefix_length>>4;
    tree_node=WINDOW+1+match_key[0]+((common_prefix_length&15)<<8);
    right_child[r]=left_child[r]=WINDOW;
    match_length=0;
    /* Preserve the paired prefix initialization: its expression value passes
     * through AX even though BC30 removes the first, unused store to parent_prefix_length.
     */
    for(common_prefix_length=parent_prefix_length=1;;) {
        if(comparison>=0) {
            if(right_child[tree_node]!=WINDOW) { tree_node=right_child[tree_node]; parent_prefix_length=prefix_length[tree_node]; }
            else { right_child[tree_node]=r; parent_node[r]=tree_node; prefix_length[r]=common_prefix_length; return; }
        } else {
            if(left_child[tree_node]!=WINDOW) { tree_node=left_child[tree_node]; parent_prefix_length=prefix_length[tree_node]; }
            else { left_child[tree_node]=r; parent_node[r]=tree_node; prefix_length[r]=common_prefix_length; return; }
        }
        if(common_prefix_length>parent_prefix_length) { common_prefix_length=parent_prefix_length; comparison=match_key[common_prefix_length]-dictionary[tree_node+common_prefix_length]; }
        else if(common_prefix_length==parent_prefix_length) {
            while(common_prefix_length<LOOKAHEAD) {
                if((comparison=match_key[common_prefix_length]-dictionary[tree_node+common_prefix_length])!=0) break;
                common_prefix_length++;
            }
        }
        if(common_prefix_length>THRESHOLD) {
            if(common_prefix_length>match_length) {
                match_position=((r-tree_node)&(WINDOW-1))-1;
                if((match_length=common_prefix_length)>=LOOKAHEAD) break;
            } else if(common_prefix_length==match_length) {
                if((candidate_distance=((r-tree_node)&(WINDOW-1))-1)<match_position) match_position=candidate_distance;
            }
        }
    }
    prefix_length[r]=prefix_length[tree_node];
    parent_node[r]=parent_node[tree_node];
    left_child[r]=left_child[tree_node]; right_child[r]=right_child[tree_node];
    parent_node[left_child[tree_node]]=r; parent_node[right_child[tree_node]]=r;
    if(right_child[parent_node[tree_node]]==tree_node) right_child[parent_node[tree_node]]=r;
    else left_child[parent_node[tree_node]]=r;
    parent_node[tree_node]=WINDOW;
}

/* Measures the shared prefix of an ancestor and node, stopping at the first differing byte.
 * Original 1A72:03A8, 156 bytes. */
void lzh_set_prefix(int prefix_length_to_check,int ancestor_node,int dictionary_node)
{
    unsigned char *ancestor_text,*node_text,*ancestor_text_end;
    if(ancestor_node>=WINDOW) prefix_length[dictionary_node]=1;
    else {
        ancestor_text=&dictionary[ancestor_node+prefix_length_to_check]; node_text=&dictionary[dictionary_node+prefix_length_to_check];
        ancestor_text_end=&dictionary[ancestor_node+LOOKAHEAD];
        while(ancestor_text<ancestor_text_end) {
            if(*ancestor_text++!=*node_text++) {
                prefix_length[dictionary_node]=ancestor_text-1-dictionary-ancestor_node;
                return;
            }
        }
        prefix_length[dictionary_node]=LOOKAHEAD;
    }
}

/* Reconciles two stored prefix lengths, recomputing the new node's prefix when required.
 * Original 1A72:0444, 105 bytes. */
void lzh_merge_prefix(int ancestor_node,int old_dictionary_node,int new_dictionary_node)
{
    int prefix_length_difference;
    if((prefix_length_difference=prefix_length[old_dictionary_node]-prefix_length[new_dictionary_node])==0)
        lzh_set_prefix(prefix_length[old_dictionary_node],ancestor_node,new_dictionary_node);
    else if(prefix_length_difference<0) prefix_length[new_dictionary_node]=prefix_length[old_dictionary_node];
}

/* Removes a dictionary position from the search tree and reconnects its child or predecessor.
 * Original 1A72:04AD, 642 bytes. */
void lzh_delete_node(int dictionary_node)
{
    int replacement_node;
    if(parent_node[dictionary_node]==WINDOW) return;
    if(right_child[dictionary_node]==WINDOW) {
        if((replacement_node=left_child[dictionary_node])!=WINDOW) lzh_merge_prefix(parent_node[dictionary_node],dictionary_node,replacement_node);
    } else if(left_child[dictionary_node]==WINDOW) {
        replacement_node=right_child[dictionary_node]; lzh_merge_prefix(parent_node[dictionary_node],dictionary_node,replacement_node);
    } else {
        replacement_node=left_child[dictionary_node];
        if(right_child[replacement_node]!=WINDOW) {
            do { replacement_node=right_child[replacement_node]; } while(right_child[replacement_node]!=WINDOW);
            if(left_child[replacement_node]!=WINDOW) lzh_merge_prefix(parent_node[replacement_node],replacement_node,left_child[replacement_node]);
            lzh_set_prefix(1,replacement_node,left_child[dictionary_node]);
            right_child[parent_node[replacement_node]]=left_child[replacement_node];
            parent_node[left_child[replacement_node]]=parent_node[replacement_node];
            left_child[replacement_node]=left_child[dictionary_node]; parent_node[left_child[dictionary_node]]=replacement_node;
        }
        lzh_set_prefix(1,parent_node[dictionary_node],replacement_node);
        lzh_set_prefix(1,replacement_node,right_child[dictionary_node]);
        right_child[replacement_node]=right_child[dictionary_node]; parent_node[right_child[dictionary_node]]=replacement_node;
    }
    parent_node[replacement_node]=parent_node[dictionary_node];
    if(right_child[parent_node[dictionary_node]]==dictionary_node) right_child[parent_node[dictionary_node]]=replacement_node;
    else left_child[parent_node[dictionary_node]]=replacement_node;
    parent_node[dictionary_node]=WINDOW;
}

/* The three readers share the same refill contract but distinct shift counts. */
#define REFILL_BITS(value) \
    value=get_buffer; \
    if(get_count<=8) { \
        input_byte=getc(lzh_input); if(input_byte<0) input_byte=0; \
        value|=input_byte<<(8-get_count); get_count+=8; \
    }

/* Refills the shared bit buffer as needed, consumes one high-order bit, and returns it.
 * Original 1A72:072F, 133 bytes. */
int lzh_get_bit(void)
{
    unsigned bit_buffer;
    int input_byte;
    REFILL_BITS(bit_buffer);
    get_buffer=bit_buffer<<1; get_count--;
    return (bit_buffer&0x8000)?1:0;
}

/* Refills the shared bit buffer as needed, consumes eight high-order bits, and returns the byte.
 * Original 1A72:07B4, 133 bytes. */
unsigned lzh_get_byte(void)
{
    unsigned bit_buffer;
    int input_byte;
    REFILL_BITS(bit_buffer);
    get_buffer=bit_buffer<<8; get_count-=8;
    return (bit_buffer>>8)&0xff;
}

/* Refills the shared bit buffer as needed, consumes the requested count, and masks the returned
 * bits.
 * Original 1A72:0839, 154 bytes. */
unsigned lzh_get_bits(int bit_count)
{
    unsigned bit_buffer;
    int input_byte;
    REFILL_BITS(bit_buffer);
    get_buffer=bit_buffer<<bit_count; get_count-=bit_count;
    return (bit_buffer>>bit_shift[bit_count])&bit_mask[bit_count];
}

/* Appends a Huffman code to the bit buffer and writes each complete output byte.
 * Original 1A72:08D3, 299 bytes. */
void lzh_put_code(int code_length,unsigned code)
{
    int buffered_bit_count;
    unsigned output_buffer;
    buffered_bit_count=put_count; output_buffer=put_buffer;
    output_buffer|=code>>buffered_bit_count;
    if((buffered_bit_count+=code_length)>=8) {
        putc(output_buffer>>8,lzh_output);
        if((buffered_bit_count-=8)>=8) {
            putc(output_buffer,lzh_output);
            compressed_size+=2;
            buffered_bit_count-=8; output_buffer=code<<(code_length-buffered_bit_count);
        } else { output_buffer<<=8; compressed_size++; }
    }
    put_buffer=output_buffer; put_count=buffered_bit_count;
}

/* Builds the initial adaptive Huffman tree and clears bit input/output state.
 * Original 1A72:09FE, 207 bytes. */
void lzh_start_huffman(void)
{
    int node_index,parent_index;
    for(node_index=0;node_index<SYMBOLS;node_index++) {
        huff_frequency[node_index]=1; huff_son[node_index]=node_index+TREE; huff_parent[node_index+TREE]=node_index;
    }
    node_index=0; parent_index=SYMBOLS;
    while(parent_index<=ROOT) {
        huff_frequency[parent_index]=huff_frequency[node_index]+huff_frequency[node_index+1];
        huff_son[parent_index]=node_index; huff_parent[node_index]=huff_parent[node_index+1]=parent_index;
        node_index+=2; parent_index++;
    }
    huff_frequency[TREE]=0xffff; huff_parent[ROOT]=0;
    put_count=get_count=0;
    put_buffer=get_buffer=0;
}

/* Rebuilds the adaptive Huffman tree with halved frequencies and restores parent links.
 * Original 1A72:0ACD, 412 bytes. */
void lzh_reconstruct_huffman(void)
{
    int node_index,parent_index,insertion_index;
    unsigned combined_frequency;
    unsigned *frequency_cursor,*insertion_limit;
    parent_index=0;
    for(node_index=0;node_index<TREE;node_index++) {
        if(huff_son[node_index]>=TREE) {
            huff_frequency[parent_index]=(huff_frequency[node_index]+1)>>1;
            huff_son[parent_index]=huff_son[node_index]; parent_index++;
        }
    }
    node_index=0; parent_index=SYMBOLS;
    /* Insert each combined frequency into the sorted prefix before rebuilding links. */
    while(parent_index<TREE) {
        insertion_index=node_index+1;
        combined_frequency=huff_frequency[parent_index]=huff_frequency[node_index]+huff_frequency[insertion_index];
        for(insertion_index=parent_index-1;combined_frequency<huff_frequency[insertion_index];insertion_index--);
        insertion_index++;
        frequency_cursor=&huff_frequency[parent_index]; insertion_limit=&huff_frequency[insertion_index];
        while(frequency_cursor>insertion_limit) { *frequency_cursor=frequency_cursor[-1]; frequency_cursor--; }
        huff_frequency[insertion_index]=combined_frequency;
        frequency_cursor=(unsigned *)&huff_son[parent_index]; insertion_limit=(unsigned *)&huff_son[insertion_index];
        while(frequency_cursor>insertion_limit) { *frequency_cursor=frequency_cursor[-1]; frequency_cursor--; }
        huff_son[insertion_index]=node_index;
        node_index+=2; parent_index++;
    }
    for(node_index=0;node_index<TREE;node_index++) {
        if((insertion_index=huff_son[node_index])>=TREE) huff_parent[insertion_index]=node_index;
        else huff_parent[insertion_index]=huff_parent[insertion_index+1]=node_index;
    }
}

/* Increments a symbol's Huffman frequency and swaps nodes as needed to preserve tree order.
 * Original 1A72:0C69, 349 bytes. */
void lzh_update_huffman(unsigned symbol)
{
    unsigned *frequency_cursor;
    int parent_index,child_node,next_node;
    unsigned updated_frequency;
    if(huff_frequency[ROOT]==0x8000) lzh_reconstruct_huffman();
    symbol=huff_parent[symbol+TREE];
    do {
        updated_frequency=++huff_frequency[symbol];
        if(updated_frequency>huff_frequency[next_node=symbol+1]) {
            frequency_cursor=&huff_frequency[next_node+1];
            while(updated_frequency>*frequency_cursor++);
            next_node=frequency_cursor-huff_frequency-2;
            huff_frequency[symbol]=frequency_cursor[-2]; frequency_cursor[-2]=updated_frequency;
            parent_index=huff_son[symbol]; huff_parent[parent_index]=next_node;
            if(parent_index<TREE) huff_parent[parent_index+1]=next_node;
            child_node=huff_son[next_node]; huff_son[next_node]=parent_index;
            huff_parent[child_node]=symbol;
            if(child_node<TREE) huff_parent[child_node+1]=symbol;
            huff_son[symbol]=child_node; symbol=next_node;
        }
    } while((symbol=huff_parent[symbol])!=0);
}

/* Traverses the adaptive Huffman tree using input bits, updates the decoded symbol, and returns
 * it.
 * Original 1A72:0DC6, 53 bytes. */
unsigned lzh_decode_character(void)
{
    unsigned decoded_symbol;
    decoded_symbol=huff_son[ROOT];
    while(decoded_symbol<TREE) { decoded_symbol+=lzh_get_bit(); decoded_symbol=huff_son[decoded_symbol]; }
    decoded_symbol-=TREE; lzh_update_huffman(decoded_symbol);
    return decoded_symbol;
}

/* Decodes a dictionary position from its prefix code and remaining input bits.
 * Original 1A72:0DFB, 69 bytes. */
unsigned lzh_decode_position(void)
{
    unsigned encoded_position,position_bit_count,position_prefix;
    encoded_position=lzh_get_byte(); position_prefix=(unsigned)position_code[encoded_position]<<6;
    position_bit_count=position_length[encoded_position]; position_bit_count-=2;
    return position_prefix|(((encoded_position<<position_bit_count)|lzh_get_bits(position_bit_count))&0x3f);
}

/* Decodes Huffman literals and dictionary matches into the output stream and sliding window.
 * Original 1A72:0E40, 530 bytes. */
void lzh_decode(void)
{
    int match_source, copy_length, copy_index, decoded_value, dictionary_cursor;
    long bytes_written;
    if (expanded_size == 0) return;
    lzh_start_huffman();
    /* BC30's loop optimizer turns this indexed byte fill into REP STOSW;
     * the memset intrinsic uses a different register setup.
     */
    for (match_source = 0; match_source < WINDOW - LOOKAHEAD; match_source++)
        dictionary[match_source] = ' ';
    dictionary_cursor = WINDOW - LOOKAHEAD;
    for (bytes_written = 0; bytes_written < expanded_size;) {
        decoded_value = lzh_decode_character();
        if (decoded_value < 256) {
            putc(decoded_value, lzh_output);
            dictionary[dictionary_cursor++] = decoded_value;
            dictionary_cursor &= WINDOW - 1;
            bytes_written++;
        } else {
            match_source = (dictionary_cursor - lzh_decode_position() - 1) & (WINDOW - 1);
            copy_length = decoded_value - 255 + THRESHOLD;
            /* Copy bytewise so a match can read bytes written earlier in this run. */
            for (copy_index = 0; copy_index < copy_length; copy_index++) {
                decoded_value = dictionary[(match_source + copy_index) & (WINDOW - 1)];
                putc(decoded_value, lzh_output);
                dictionary[dictionary_cursor++] = decoded_value;
                dictionary_cursor &= WINDOW - 1;
                bytes_written++;
            }
        }
        if (bytes_written > progress_next) {
            putc(':', &extraction_stdout);
            fflush(&extraction_stdout);
            progress_next += progress_step;
        }
    }
}

/* Prints extraction progress and computes the interval between progress marks from file length.
 * Original 1A72:1052, 316 bytes. */
void lzh_progress_begin(char *filename,long length,char *message)
{
    int dot_columns;
    long dot_count;
    dot_columns=64-strlen(filename); if(dot_columns<0) dot_columns=3;
    printf(progress_format,filename,message);
    progress_step=((((long)dot_columns<<12)+length-1)/((long)dot_columns<<12))<<12;
    dot_count=(progress_step+length-1)/progress_step;
    while(dot_count--) putc('.',&extraction_stdout);
    progress_next=0;
    printf(progress_format,filename,message);
    fflush(&extraction_stdout);
}

/* Prints the completed extraction message and flushes standard output.
 * Original 1A72:118E, 45 bytes. */
void lzh_progress_end(char *filename,char *message)
{
    printf(complete_format,filename,message);
    fflush(&extraction_stdout);
}

/* Verifies executable metadata, opens the appended LZH stream and DAZZLE.DOC, decodes it, and
 * exits.
 * Original 1A72:11BB, 485 bytes. */
void extract_manual(void)
{
    progress_next=0; progress_step=1024;
    expanded_size=compressed_size=0;
    insecure_argument_index=0; check_executable_integrity();
    if(image_data_offset<=1) lzh_fatal(resource_error);
    if(!(lzh_input=fopen(executable_path,read_mode)) ||
       !(lzh_output=fopen(manual_filename,write_mode))) lzh_fatal(open_error);
    read_mz_resource_base(lzh_input); resource_base+=10;
    if(fseek(lzh_input,resource_base,SEEK_SET)) lzh_fatal(seek_error);
    if(fread(&expanded_size,4,1,lzh_input)<1) lzh_fatal(length_error);
    lzh_progress_begin(executable_path,expanded_size,extract_message);
    lzh_decode();
    lzh_progress_end(manual_filename,complete_message);
    fclose(lzh_input);
    if(fclose(lzh_output)==EOF) lzh_fatal(close_error);
    exit(501);
}

/* LZH position decoding and bit input/output state. */
unsigned char position_code[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9,
    10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 13, 13, 13, 13,
    14, 14, 14, 14, 15, 15, 15, 15,
    16, 16, 16, 16, 17, 17, 17, 17,
    18, 18, 18, 18, 19, 19, 19, 19,
    20, 20, 20, 20, 21, 21, 21, 21,
    22, 22, 22, 22, 23, 23, 23, 23,
    24, 24, 25, 25, 26, 26, 27, 27,
    28, 28, 29, 29, 30, 30, 31, 31,
    32, 32, 33, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 38, 38, 39, 39,
    40, 40, 41, 41, 42, 42, 43, 43,
    44, 44, 45, 45, 46, 46, 47, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};
unsigned char position_length[256] = {
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8
};
unsigned get_buffer = 0;
unsigned char get_count = 0;
unsigned bit_mask[17] = {
    0, 1, 3, 7, 15, 31, 63, 127,
    255, 511, 1023, 2047, 4095, 8191, 16383, 4095,
    65535
};
unsigned bit_shift[17] = {
    16, 15, 14, 13, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1,
    0
};
unsigned put_buffer = 0;
unsigned char put_count = 0;
extern char manual_document_name[];
char *manual_filename = manual_document_name;
char fatal_banner[] = "\r Program aborting in text extraction routines!\nThis copy may be corrupted, write us for a good copy.\n";
char fatal_format[] = "REASON: %s \n\a";
char progress_format[] = "\r%s\t- %s :  ";
char complete_format[] = "\n%s\t- %s\n";
char manual_document_name[] = "DAZZLE.DOC";
char resource_error[] = "getLZH data missing";
char read_mode[] = "rb";
char write_mode[] = "wb";
char open_error[] = "getLZH file access error";
char seek_error[] = "getLZH file seek error";
char length_error[] = "getLZH cannot read textsize from input";
char extract_message[] = "Extracting ";
char complete_message[] = "Extracted ";
char close_error[] = "error during close of output file";
