#include <stdio.h>
#include <math.h>
#include "../include/tree.h"

typedef struct {
    void* val; // Holds data
    size_t data_size; // Size of data
    unsigned long long count; // Number of equivalent items
    unsigned long long height; // Height of node
    Node* left; // Left child
    Node* right; // Right chil
} Node;


typedef struct Tree {
    Node* root; // Root of tree
    int (*compare)(const void*, const void*); // Comparison function
    unsigned long long size; // Number of items in tree
    unsigned long long num_nodes; // Number of unique items in tre
} Tree;


Node* rotate_left(Node* node) {
    // Get rotated nodes
    Node* r = node->right;
    Node* rl = r->left;
    
    // Perform rotations
    r->left = node;
    node->right = rl;

    // Update heights
    node->height =1 + fmax(
        node->left ? node->left->height : 0,
        node->right ? node->right->height : 0
    );
    r->height = 1 + fmax(
        r->left ? r->left->height : 0,
        r->right ? r->right->height : 0
    );

    return r;
}


Node* rotate_left(Node* node) {
    Node* r = node->right;
    Node* rl = r->left;

    // Perform rotation
    r->left = node;
    node->right = rl;

    // Update heights
    node->height = 1 + fmax(
        node->left ? node->left->height : 0,
        node->right ? node->right->height : 0
    );
    r->height = 1 + fmax(
        r->left ? r->left->height : 0,
        r->right ? r->right->height : 0
    );

    return r; // return new root of this subtree
}


Node* rotate_ll(Node* node) {
    return rotate_right(node);
}


Node* rotate_rr(Node* node) {
    return rotate_left(node);
}


Node* rotate_lr(Node* node) {
    node->left = rotate_left(node->left);
    return rotate_right(node);
}


Node* rotate_rl(Node* node) {
    node->right = rotate_right(node->right);
    return rotate_left(node);
}


Node* balance(Node* node) {

}


Node* node_create(const void* val, size_t data_size) {
    Node* node = malloc(sizeof(Node)); // Allocate memory

    if(!node) // Allocation failed
        return NULL;

    node->val = malloc(data_size); // Allocate memory for data

    if (!node->val) { // Allocation failed
        free(node);
        return NULL;
    }

    // Copy and save data to node
    memcpy(node->val, val, data_size);
    node->data_size = data_size;

    // Initialize node's fields
    node->count = 1;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
}


Tree* tree_create(int (*compare)(const void*, const void*)) {
    Tree* tree = malloc(sizeof(Tree)); // Allocate memory

    if(!tree) // Allocation failed
        return NULL;

    // Initialize fields
    tree->root = NULL;
    tree->compare = compare;
    tree->size = 0;
    tree->num_nodes = 0;

    return tree;
}


void node_insert(Node* node, void* val, size_t size, int (*compare)(const void *, const void *)) {
    int comparison = compare(node->val, val);

    if(comparison < 0) { // Search left subtree
        if(!node->left) { // Insert location found 
            node->left = node_create(val, size);
            node->height++;
            return 1;
        }

        // Continue search
        node_insert(node->left, val, size, compare);
    } else if(comparison > 0) { // Search right subtree
        if(!node->right) { // Insert location found 
            node->right = node_create(val, size);
            node->height++;
            return 1;
        }

        // Continue search
        node_insert(node->right, val, size, compare);
    } else { // Value already exists
        node->count++;
        return 1;
    }
}


void tree_insert(Tree* tree, void* val, size_t size) {
    if(!tree || !val)
        return 0;


    if(!tree->root) { // Tree is empty, set root
        Node* node = node_create(val, size);

        if(!node) // Node creation failed
            return 0;

        // Set node as root
        tree->root = node;
        return 1;
    }

    node_insert(tree->root, val, size, tree->compare);
}

