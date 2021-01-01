#include <pongo.h>

static inline dt_prop_t *skip_props(dt_prop_t *prop, uint32_t nprop) {
    for (uint32_t i = 0; i < nprop; i++) {
        prop = (dt_prop_t *)(prop->val + ((prop->len + 0x3) & ~0x3));
    } 
    return prop;
}

static dt_node_t *skip_node(dt_node_t *cur_node) {

    // skip props
    dt_prop_t *cur_prop = (dt_prop_t *)&cur_node->prop;
    cur_prop = skip_props(cur_prop, cur_node->nprop);

    // skip childs
    uint32_t nchilds = cur_node->nchld;
    cur_node = (dt_node_t *)cur_prop;
    for (uint32_t i = 0; i < nchilds; i++) {
        cur_node = skip_node(cur_node);
    }

    return cur_node;
}

// this will overwrite any children. add props first, then children
bool dt_add_prop(dt_node_t *node, char *name, void *val, uint32_t len, void *end) {
    if (strlen(name) >= DT_KEY_LEN) {
        return false;
    }

    dt_prop_t *cur_prop = (dt_prop_t *)&node->prop;
    // skip existing props
    cur_prop = skip_props(cur_prop, node->nprop);

    // bounds check
    if (((uint64_t)cur_prop + sizeof(dt_prop_t) + len) > (uint64_t)end) {
        return false;
    }

    strncpy(cur_prop->key, name, DT_KEY_LEN);
    cur_prop->key[DT_KEY_LEN - 1] = '\0';
    cur_prop->len = len;
    memcpy(cur_prop->val, val, len);
    node->nprop += 1;

    return true;
}

bool dt_add_string_prop(dt_node_t *node, char *name, char *val, void *end) {
    return dt_add_prop(node, name, val, strlen(val) + 1, end);
}

bool dt_add_child(dt_node_t *node, char *name, dt_node_t **store_node, void *end) {
    // determine the end of the node
    dt_node_t *new_node = skip_node(node);

    // bounds check
    if ((uint64_t)new_node + sizeof(dt_node_t) > (uint64_t)end) {
        return false;
    }
    new_node->nprop = 0;
    new_node->nprop = 0;
    node->nchld += 1;

    if (!dt_add_string_prop(new_node, "name", name, end)) {
        return false;
    }

    *store_node = new_node;
    return true;
}