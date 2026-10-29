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

/* objcmp: {"function":"lzh_code_segment","address":"1a72:0007","size":3,"symbols":{"_exit_requested":"29bf:5d80","_extraction_stdout":"29bf:4408","_extraction_stderr":"29bf:441c","_lzh_input":"1e81:b3dc","_lzh_output":"1e81:b3d8","_prefix_length":"1e81:1382","_right_child":"1e81:2383","_parent_node":"1e81:6385","_left_child":"1e81:8387","_match_length":"1e81:a389","_match_position":"1e81:a38b","_dictionary":"1e81:a38d","_huff_son":"1e81:025a","_huff_parent":"1e81:0740","_huff_frequency":"1e81:0e9a","_progress_step":"1e81:b3c8","_progress_next":"1e81:b3cc","_compressed_size":"1e81:b3d0","_expanded_size":"1e81:b3d4","_get_buffer":"DS:3f66","_get_count":"DS:3f68","_put_buffer":"DS:3fad","_put_count":"DS:3faf","_bit_mask":"DS:3f69","_bit_shift":"DS:3f8b","_position_code":"DS:3d66","_position_length":"DS:3e66","_manual_filename":"DS:3fb0","_fatal_banner":"DS:3fb4","_fatal_format":"DS:401b","_progress_format":"DS:4029","_complete_format":"DS:4036","_resource_error":"DS:404b","_read_mode":"DS:405f","_write_mode":"DS:4062","_open_error":"DS:4065","_seek_error":"DS:407e","_length_error":"DS:4095","_extract_message":"DS:40bc","_complete_message":"DS:40c8","_close_error":"DS:40d3","_executable_path":"29bf:5d17","_insecure_argument_index":"29bf:5dc8","_image_data_offset":"29bf:602f","_resource_base":"DS:5d68","_print_order_info":"11c1:0ab6","_check_executable_integrity":"11c1:0949","_read_mz_resource_base":"11c1:05c3","_fprintf":"0000:45d4","_printf":"0000:4fb3","_exit":"0000:11a8","__fgetc":"0000:4a0d","__fputc":"0000:4fce","_fflush":"0000:4169","_fopen":"0000:459f","_fseek":"0000:47f4","_fread":"0000:46e4","_fclose":"0000:407f","_strlen":"0000:5a36","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_executable_name_pointer":"DS:0096","_fade_records":"DS:0474","_background_fill_word":"DS:5d8a","_g_5d92":"DS:5d92","_palette_buffer":"DS:5d94","_image_source_bytes":"DS:7456","_selfcheck_stream":"DS:6002","_mode_setup_callback":"DS:5c43","_saved_text_info":"DS:6016","__streams":"DS:43f4","_saved_keyboard_vector":"DS:5ff2","_saved_timer_vector":"DS:5ff6","_palette_base_green":"DS:5d6c"},"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
unsigned lzh_code_segment(void) { return _CS; }

/* objcmp: {"function":"lzh_fatal","address":"1a72:000a","size":84,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_fatal(char *reason)
{
    exit_requested=1;
    fprintf(&extraction_stderr,fatal_banner);
    fprintf(&extraction_stderr,fatal_format,reason);
    print_order_info(0);
    exit(500);
}

/* objcmp: {"function":"lzh_init_dictionary_tree","address":"1a72:005e","size":100,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_init_dictionary_tree(void)
{
    int far *p,*end;
    p=&right_child[WINDOW+1]; end=&right_child[WINDOW*2];
    while(p<=end) *p++=WINDOW;
    p=parent_node; end=&parent_node[WINDOW];
    while(p<end) *p++=WINDOW;
}

/* objcmp: {"function":"lzh_insert_node","address":"1a72:00c2","size":742,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_insert_node(int r)
{
    int p,cmp;
    unsigned char *key;
    unsigned distance,common,i;
    cmp=1; key=&dictionary[r];
    common=key[1]^key[2]; common^=common>>4;
    p=WINDOW+1+key[0]+((common&15)<<8);
    right_child[r]=left_child[r]=WINDOW;
    match_length=0;
    /* Preserve the paired prefix initialization: its expression value passes
     * through AX even though BC30 removes the first, unused store to i.
     */
    for(common=i=1;;) {
        if(cmp>=0) {
            if(right_child[p]!=WINDOW) { p=right_child[p]; i=prefix_length[p]; }
            else { right_child[p]=r; parent_node[r]=p; prefix_length[r]=common; return; }
        } else {
            if(left_child[p]!=WINDOW) { p=left_child[p]; i=prefix_length[p]; }
            else { left_child[p]=r; parent_node[r]=p; prefix_length[r]=common; return; }
        }
        if(common>i) { common=i; cmp=key[common]-dictionary[p+common]; }
        else if(common==i) {
            while(common<LOOKAHEAD) {
                if((cmp=key[common]-dictionary[p+common])!=0) break;
                common++;
            }
        }
        if(common>THRESHOLD) {
            if(common>match_length) {
                match_position=((r-p)&(WINDOW-1))-1;
                if((match_length=common)>=LOOKAHEAD) break;
            } else if(common==match_length) {
                if((distance=((r-p)&(WINDOW-1))-1)<match_position) match_position=distance;
            }
        }
    }
    prefix_length[r]=prefix_length[p];
    parent_node[r]=parent_node[p];
    left_child[r]=left_child[p]; right_child[r]=right_child[p];
    parent_node[left_child[p]]=r; parent_node[right_child[p]]=r;
    if(right_child[parent_node[p]]==p) right_child[parent_node[p]]=r;
    else left_child[parent_node[p]]=r;
    parent_node[p]=WINDOW;
}

/* objcmp: {"function":"lzh_set_prefix","address":"1a72:03a8","size":156,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_set_prefix(int length,int ancestor,int node)
{
    unsigned char *a,*b,*end;
    if(ancestor>=WINDOW) prefix_length[node]=1;
    else {
        a=&dictionary[ancestor+length]; b=&dictionary[node+length];
        end=&dictionary[ancestor+LOOKAHEAD];
        while(a<end) {
            if(*a++!=*b++) {
                prefix_length[node]=a-1-dictionary-ancestor;
                return;
            }
        }
        prefix_length[node]=LOOKAHEAD;
    }
}

/* objcmp: {"function":"lzh_merge_prefix","address":"1a72:0444","size":105,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_merge_prefix(int ancestor,int old_node,int new_node)
{
    int difference;
    if((difference=prefix_length[old_node]-prefix_length[new_node])==0)
        lzh_set_prefix(prefix_length[old_node],ancestor,new_node);
    else if(difference<0) prefix_length[new_node]=prefix_length[old_node];
}

/* objcmp: {"function":"lzh_delete_node","address":"1a72:04ad","size":642,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_delete_node(int p)
{
    int q;
    if(parent_node[p]==WINDOW) return;
    if(right_child[p]==WINDOW) {
        if((q=left_child[p])!=WINDOW) lzh_merge_prefix(parent_node[p],p,q);
    } else if(left_child[p]==WINDOW) {
        q=right_child[p]; lzh_merge_prefix(parent_node[p],p,q);
    } else {
        q=left_child[p];
        if(right_child[q]!=WINDOW) {
            do { q=right_child[q]; } while(right_child[q]!=WINDOW);
            if(left_child[q]!=WINDOW) lzh_merge_prefix(parent_node[q],q,left_child[q]);
            lzh_set_prefix(1,q,left_child[p]);
            right_child[parent_node[q]]=left_child[q];
            parent_node[left_child[q]]=parent_node[q];
            left_child[q]=left_child[p]; parent_node[left_child[p]]=q;
        }
        lzh_set_prefix(1,parent_node[p],q);
        lzh_set_prefix(1,q,right_child[p]);
        right_child[q]=right_child[p]; parent_node[right_child[p]]=q;
    }
    parent_node[q]=parent_node[p];
    if(right_child[parent_node[p]]==p) right_child[parent_node[p]]=q;
    else left_child[parent_node[p]]=q;
    parent_node[p]=WINDOW;
}

/* The three readers share the same refill contract but distinct shift counts. */
#define REFILL_BITS(value) \
    value=get_buffer; \
    if(get_count<=8) { \
        c=getc(lzh_input); if(c<0) c=0; \
        value|=c<<(8-get_count); get_count+=8; \
    }

/* objcmp: {"function":"lzh_get_bit","address":"1a72:072f","size":133,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
int lzh_get_bit(void)
{
    unsigned value;
    int c;
    REFILL_BITS(value);
    get_buffer=value<<1; get_count--;
    return (value&0x8000)?1:0;
}

/* objcmp: {"function":"lzh_get_byte","address":"1a72:07b4","size":133,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
unsigned lzh_get_byte(void)
{
    unsigned value;
    int c;
    REFILL_BITS(value);
    get_buffer=value<<8; get_count-=8;
    return (value>>8)&0xff;
}

/* objcmp: {"function":"lzh_get_bits","address":"1a72:0839","size":154,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
unsigned lzh_get_bits(int count)
{
    unsigned value;
    int c;
    REFILL_BITS(value);
    get_buffer=value<<count; get_count-=count;
    return (value>>bit_shift[count])&bit_mask[count];
}

/* objcmp: {"function":"lzh_put_code","address":"1a72:08d3","size":299,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_put_code(int length,unsigned code)
{
    int count;
    unsigned buffer;
    count=put_count; buffer=put_buffer;
    buffer|=code>>count;
    if((count+=length)>=8) {
        putc(buffer>>8,lzh_output);
        if((count-=8)>=8) {
            putc(buffer,lzh_output);
            compressed_size+=2;
            count-=8; buffer=code<<(length-count);
        } else { buffer<<=8; compressed_size++; }
    }
    put_buffer=buffer; put_count=count;
}

/* objcmp: {"function":"lzh_start_huffman","address":"1a72:09fe","size":207,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_start_huffman(void)
{
    int i,j;
    for(i=0;i<SYMBOLS;i++) {
        huff_frequency[i]=1; huff_son[i]=i+TREE; huff_parent[i+TREE]=i;
    }
    i=0; j=SYMBOLS;
    while(j<=ROOT) {
        huff_frequency[j]=huff_frequency[i]+huff_frequency[i+1];
        huff_son[j]=i; huff_parent[i]=huff_parent[i+1]=j;
        i+=2; j++;
    }
    huff_frequency[TREE]=0xffff; huff_parent[ROOT]=0;
    put_count=get_count=0;
    put_buffer=get_buffer=0;
}

/* objcmp: {"function":"lzh_reconstruct_huffman","address":"1a72:0acd","size":412,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_reconstruct_huffman(void)
{
    int i,j,k;
    unsigned f;
    unsigned *p,*end;
    j=0;
    for(i=0;i<TREE;i++) {
        if(huff_son[i]>=TREE) {
            huff_frequency[j]=(huff_frequency[i]+1)>>1;
            huff_son[j]=huff_son[i]; j++;
        }
    }
    i=0; j=SYMBOLS;
    while(j<TREE) {
        k=i+1;
        f=huff_frequency[j]=huff_frequency[i]+huff_frequency[k];
        for(k=j-1;f<huff_frequency[k];k--);
        k++;
        p=&huff_frequency[j]; end=&huff_frequency[k];
        while(p>end) { *p=p[-1]; p--; }
        huff_frequency[k]=f;
        p=(unsigned *)&huff_son[j]; end=(unsigned *)&huff_son[k];
        while(p>end) { *p=p[-1]; p--; }
        huff_son[k]=i;
        i+=2; j++;
    }
    for(i=0;i<TREE;i++) {
        if((k=huff_son[i])>=TREE) huff_parent[k]=i;
        else huff_parent[k]=huff_parent[k+1]=i;
    }
}

/* objcmp: {"function":"lzh_update_huffman","address":"1a72:0c69","size":349,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_update_huffman(unsigned i)
{
    unsigned *p;
    int j,k,l;
    unsigned frequency;
    if(huff_frequency[ROOT]==0x8000) lzh_reconstruct_huffman();
    i=huff_parent[i+TREE];
    do {
        frequency=++huff_frequency[i];
        if(frequency>huff_frequency[l=i+1]) {
            p=&huff_frequency[l+1];
            while(frequency>*p++);
            l=p-huff_frequency-2;
            huff_frequency[i]=p[-2]; p[-2]=frequency;
            j=huff_son[i]; huff_parent[j]=l;
            if(j<TREE) huff_parent[j+1]=l;
            k=huff_son[l]; huff_son[l]=j;
            huff_parent[k]=i;
            if(k<TREE) huff_parent[k+1]=i;
            huff_son[i]=k; i=l;
        }
    } while((i=huff_parent[i])!=0);
}

/* objcmp: {"function":"lzh_decode_character","address":"1a72:0dc6","size":53,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
unsigned lzh_decode_character(void)
{
    unsigned symbol;
    symbol=huff_son[ROOT];
    while(symbol<TREE) { symbol+=lzh_get_bit(); symbol=huff_son[symbol]; }
    symbol-=TREE; lzh_update_huffman(symbol);
    return symbol;
}

/* objcmp: {"function":"lzh_decode_position","address":"1a72:0dfb","size":69,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
unsigned lzh_decode_position(void)
{
    unsigned i,j,high;
    i=lzh_get_byte(); high=(unsigned)position_code[i]<<6;
    j=position_length[i]; j-=2;
    return high|(((i<<j)|lzh_get_bits(j))&0x3f);
}

/* objcmp: {"function":"lzh_decode","address":"1a72:0e40","size":530,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_decode(void)
{
    int i,j,k,c,r;
    long count;
    if(expanded_size==0) return;
    lzh_start_huffman();
    /* BC30's loop optimizer turns this indexed byte fill into REP STOSW;
     * the memset intrinsic uses a different register setup.
     */
    for(i=0;i<WINDOW-LOOKAHEAD;i++) dictionary[i]=' ';
    r=WINDOW-LOOKAHEAD;
    for(count=0;count<expanded_size;) {
        c=lzh_decode_character();
        if(c<256) {
            putc(c,lzh_output);
            dictionary[r++]=c; r&=WINDOW-1; count++;
        } else {
            i=(r-lzh_decode_position()-1)&(WINDOW-1);
            j=c-255+THRESHOLD;
            for(k=0;k<j;k++) {
                c=dictionary[(i+k)&(WINDOW-1)];
                putc(c,lzh_output);
                dictionary[r++]=c; r&=WINDOW-1; count++;
            }
        }
        if(count>progress_next) {
            putc(':',&extraction_stdout); fflush(&extraction_stdout);
            progress_next+=progress_step;
        }
    }
}

/* objcmp: {"function":"lzh_progress_begin","address":"1a72:1052","size":316,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_progress_begin(char *filename,long length,char *message)
{
    int columns;
    long count;
    columns=64-strlen(filename); if(columns<0) columns=3;
    printf(progress_format,filename,message);
    progress_step=((((long)columns<<12)+length-1)/((long)columns<<12))<<12;
    count=(progress_step+length-1)/progress_step;
    while(count--) putc('.',&extraction_stdout);
    progress_next=0;
    printf(progress_format,filename,message);
    fflush(&extraction_stdout);
}

/* objcmp: {"function":"lzh_progress_end","address":"1a72:118e","size":45,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
void lzh_progress_end(char *filename,char *message)
{
    printf(complete_format,filename,message);
    fflush(&extraction_stdout);
}

/* objcmp: {"function":"extract_manual","address":"1a72:11bb","size":485,"segments":{"_DATA":"29bf:3d66","FAR_DATA":"1e81:025a"}} */
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
