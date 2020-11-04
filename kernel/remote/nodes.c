#include "types.h"
#include "defs.h"
#include "nodes.h"

struct node_map nodemap;

uint64_t nodemap_hash(uint64_t nid) {
        return nid % NODE_HASH_NUM;
}

int nodemap_add(uint64_t nid) {
        uint64_t hash = nodemap_hash(nid);

        struct node* n;
        n = (struct node*) malloc(sizeof(struct node));
        n->nid = nid;
        n->next = 0;

        acquire(&nodemap.lock);
        if (nodemap.n[hash] == 0) {
                nodemap.n[hash] = n;
        } else {
                struct node* now = nodemap.n[hash];
                while (now->next != 0) {
                        now = now->next;
                }
                now->next = n;
        }
        release(&nodemap.lock);

        return 0;
}
int nodemap_remove(uint64_t nid) {
        uint64_t hash = nodemap_hash(nid);

        acquire(&nodemap.lock);
        if (nodemap.n[hash] == 0) {
                return -1;
        } else {
                struct node* now = nodemap.n[hash];
                struct node* prev = 0;
                while (now->next != 0) {
                        if (now->nid == nid) {
                                if (prev != 0)
                                        prev->next = now->next;
                                return 0;
                        } 
                        prev = now;
                        now = now->next;
                }
                return -1;
        }
        release(&nodemap.lock);
}

void node_init() {
        initlock(&nodemap.lock, "nodelist lock");
        memset(&nodemap, 0, sizeof(nodemap));
}

int node_add(uint64_t nid) {
        return nodemap_add(nid);
}

int node_remove(uint64_t nid) {
        return nodemap_remove(nid);
}
