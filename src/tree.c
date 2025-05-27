#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../include/tree.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef struct Node Node;

typedef struct Node {
    // Node key
    void* key;
    size_t key_size;

    // Node value
    void* val;
    size_t val_size;

    // Tree structure data
    unsigned long long height;
    Node* left;
    Node* right;
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
    node->height =1 + MAX(
        node->left ? node->left->height : 0,
        node->right ? node->right->height : 0
    );
    r->height = 1 + MAX(
        r->left ? r->left->height : 0,
        r->right ? r->right->height : 0
    );

    return r;
}


Node* rotate_right(Node* node) {
    Node* l = node->left;
    Node* lr = l->right;

    // Perform rotation
    l->right = node;
    node->left = lr;

    // Update heights
    node->height = 1 + MAX(
        node->left ? node->left->height : 0,
        node->right ? node->right->height : 0
    );
    l->height = 1 + MAX(
        l->left ? l->left->height : 0,
        l->right ? l->right->height : 0
    );

    return l;
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
    if(!node) // Input NULL
        return NULL;

    // Update height
    unsigned long long left_height = node->left ? node->left->height : 0;
    unsigned long long right_height = node->right ? node->right->height : 0;
    node->height = 1 + MAX(left_height, right_height);

    // Calculate balance factor
    int balance_factor = (int)(left_height - right_height);

    // Left heavy
    if (balance_factor > 1) {
        int left_balance = (int)((node->left->left ? node->left->left->height : 0) -
                                 (node->left->right ? node->left->right->height : 0));
        if (left_balance >= 0) {
            return rotate_right(node); // LL case
        } else {
            return rotate_lr(node); // LR case
        }
    }

    // Right heavy
    if(balance_factor < -1) {
        int right_balance = (int)((node->right->left ? node->right->left->height : 0) -
                                  (node->right->right ? node->right->right->height : 0));
        if(right_balance <= 0) {
            return rotate_left(node); // RR case
        } else {
            return rotate_rl(node); // RL case
        }
    }

    return node; // Already balanced
}


// Creat node with passed data
Node* node_create(const void* key, const void* val, size_t key_size, size_t val_size) {
    Node* node = malloc(sizeof(Node)); // Allocate memory

    if(!node) // Allocation failed
        return NULL;

    // Allocate memory for data
    node->key = malloc(key_size);
    node->val = malloc(val_size);

    if(!node->key || !node->val) { // Allocation failed
        free(node);
        return NULL;
    }

    // Copy key and value to node
    memcpy(node->key, key, key_size);
    if(val && val_size > 0)
        memcpy(node->val, val, val_size);

    // Save size of key and value
    node->key_size = key_size;
    node->val_size = val_size;

    // Initialize tree structure fields
    node->height = 1;
    node->left = NULL;
    node->right = NULL;

    return node;
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


char node_set(Node* node, const void* key, const size_t key_size, int(*set_val)(void**, size_t*), int (*compare)(const void*, const void*)) {
    int cmp = compare(key, node->key); 

    if(cmp < 0) { // Search left subtree
        if(node->left) {// Continue search
            if(node_set(node->left, key, key_size, set_val, compare)) {
                // New node created
                node->left = balance(node->left);
                return 1;
            }

            return 0;
        }

        // Create new node as left child
        node->left = node_create(key, NULL, key_size, 0); // Create new node
        set_val(&node->left->val, &node->left->val_size); // Allow caller to set val
        return 1;
    } else if(cmp > 0) { // Search right subtree
        if(node->right) {// Continue search
            if(node_set(node->right, key, key_size, set_val, compare)) {
                // New node created
                node->right = balance(node->right);
                return 1;
            }
 
            return 0;
        }

        // Create new node as right child
        node->right = node_create(key, NULL, key_size, 0); // Create new node
        set_val(&node->right->val, &node->right->val_size); // Allow caller to set val
        return 1;
    } else { // key found
        set_val(&node->val, &node->val_size); // Update value
        return 0;
    }
}


char tree_set(Tree* tree, const void* key, const size_t key_size, int (*set_val)(void**, size_t*)) {
    if(!tree || !key || !set_val)
        return 0;  // Invalid input

    if(!tree->root){ // Tree is empty
        // Set root node
        tree->root = node_create(key, NULL, key_size, 0);
        set_val(&tree->root->val, &tree->root->val_size);
        
        return 0;
    }

    
    if(node_set(tree->root, key, key_size, set_val, tree->compare)) {
        tree->size++;
        return 0;
    }

    return 1;
}


void node_print(Node* node, void (*print)(const void*, const void*, const size_t, const size_t)) {
    if(!node) // Node null
        return;

    node_print(node->left, print); // Search left
    print(node->key, node->val, node->key_size, node->val_size); // Print val using passed function
    node_print(node->right, print); // Search right
}

void tree_print(Tree* tree, void (*print)(const void*, const void*, const size_t, const size_t)) {
    if(!tree || !print)
        return; // Inputs null
    
    node_print(tree->root, print); // Print tree
}
/*
void node_write(FILE *fp, Node *node) {
    if(!node) { // Node null
        //Mark as null
        char is_null = 0;
        fwrite(&is_null, 1, 1, fp);
        return;
    }

    char is_null = 1;
    fwrite(&is_null, 1, 1, fp);

    // Write node data
    fwrite(&node->data_size, sizeof(size_t), 1, fp);
    fwrite(node->val, node->data_size, 1, fp);
    fwrite(&node->count, sizeof(unsigned long long), 1, fp);
    fwrite(&node->height, sizeof(unsigned long long), 1, fp);

    // Recursively write left and right
    node_write(fp, node->left);
    node_write(fp, node->right);
}


void tree_write(const Tree* tree, const char* filename) {
    FILE *fp = fopen(filename, "wb"); // Open file
    if(!fp) { // File open failed
        printf("Failed to open %s\n", filename);
        return;
    }

    // Write basic tree metadata (size, num_nodes)
    fwrite(&tree->size, sizeof(unsigned long long), 1, fp);
    fwrite(&tree->num_nodes, sizeof(unsigned long long), 1, fp);

    // Recursively write root node
    node_write(fp, tree->root);

    fclose(fp); // Close file
}

*/