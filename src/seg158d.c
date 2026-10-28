#include <dos.h>

struct node {
    struct node far *next;
};

extern struct node far *head;
extern void far free(void far *);

/* objcmp: {"function":"free_nodes","address":"158d:00a2","size":93,"symbols":{"_head":"DS:24b4","_free":"0000:2b47"}} */
void far free_nodes(void)
{
    struct node far *p, *old;
    p = head;
    if (head) {
        do {
            old = p;
            p = p->next;
            free(old);
        } while (p);
    }
    head = 0;
}

/* objcmp: {"function":"render_menu_text","address":"158d:0219","size":64} */
void far render_menu_text(unsigned x, unsigned y, char c, char attr)
{
    union {
        unsigned word;
        char byte[2];
    } cell;

    if (y < 25 && x < 80) {
        cell.byte[1] = attr;
        cell.byte[0] = c;
        *(unsigned far *)MK_FP(0xb800, y * 160 + x * 2) = cell.word;
    }
}

struct image_size_record {
    unsigned size;
    unsigned char reserved[13];
};

extern void far menu_error_message(char far *message);

#define IMAGE_COUNT (*(unsigned far *)MK_FP(0x29bf, 0x5d9c))
#define OTHER_IMAGE_COUNT (*(unsigned far *)MK_FP(0x29bf, 0x5d9a))
#define ALT_IMAGE_SIZES ((struct image_size_record far *)MK_FP(0x29bf, 0x0202))
#define CTRL_IMAGE_SIZES ((struct image_size_record far *)MK_FP(0x29bf, 0x041e))

/* objcmp: {"function":"check_any_image_configured","address":"158d:0c09","size":115,"symbols":{"_menu_error_message":"158d:2492"},"level":"PROVISIONAL","blocker":"segment-register-reload at 158d:0c33"} */
int far check_any_image_configured(void)
{
    unsigned index;

    for (index = 0; index < IMAGE_COUNT + OTHER_IMAGE_COUNT; ++index) {
        if (index < IMAGE_COUNT) {
            if (ALT_IMAGE_SIZES[index].size > 0)
                return 1;
        } else if (CTRL_IMAGE_SIZES[index - IMAGE_COUNT - 1].size > 0) {
            return 1;
        }
    }
    menu_error_message((char far *)MK_FP(0x29bf, 0x38e9));
    return 0;
}
