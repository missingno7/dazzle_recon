/* 158d:00a2; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 93 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "158d:00a2", "size": 93, "symbols": {"_head": "DS:24b4", "_free": "0000:2b47"}} */
struct node {struct node *next;};
extern struct node *head;
void free(void *);
void free_nodes(void) {
    struct node *p,*old;
    p=head;
    if(head) {
        do { old=p; p=p->next; free(old); } while(p);
    }
    head=0;
}
