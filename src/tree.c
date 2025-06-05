#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
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
    uint8_t height;
    Node* left;
    Node* right;
} Node;


typedef struct Tree {
    Node* root; // Root of tree
    int (*compare)(const void*, const void*); // Comparison function
    uint32_t size; // Number of items in tree
    uint8_t max_height;
} Tree;


    typedef struct TreeIter {
        Node** node_stack;
        uint8_t stack_size;
        uint8_t stack_capacity;
    } TreeIter;


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
    uint8_t left_height = node->left ? node->left->height : 0;
    uint8_t right_height = node->right ? node->right->height : 0;
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


Node* node_create(const void* key, const void* val, size_t key_size, size_t val_size) {
    if(!key) // Ensure key is not null
        return NULL;

    Node* node = malloc(sizeof(Node));
    if(!node)
        return NULL;

    node->key = malloc(key_size);
    
    if(!node->key) {
        free(node);
        return NULL;
    }

    memcpy(node->key, key, key_size);
    node->key_size = key_size;

    if(val && val_size > 0) {
        node->val = malloc(val_size);
        if(!node->val) {
            free(node->key);
            free(node);
            return NULL;
        }
        memcpy(node->val, val, val_size);
        node->val_size = val_size;
    } else {
        node->val = NULL;
        node->val_size = 0;
    }

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
    tree->max_height = 0;

    return tree;
}


char node_set(Node* node, const void* key, const size_t key_size, int(*set_val)(void**, size_t*), Tree* tree) {
    int cmp = tree->compare(key, node->key); 

    if(cmp < 0) { // Search left subtree
        if(node->left) {// Continue search
            if(node_set(node->left, key, key_size, set_val, tree)) {
                // New node created
                node->left = balance(node->left);
                return 1;
            }

            return 0;
        }

        // Create new node as left child
        node->left = node_create(key, NULL, key_size, 0); // Create new node
        set_val(&node->left->val, &node->left->val_size); // Allow caller to set val

        //Increment tree height if necessary
        if(node->height == tree->max_height)
            tree->max_height++;

        return 1;
    } else if(cmp > 0) { // Search right subtree
        if(node->right) {// Continue search
            if(node_set(node->right, key, key_size, set_val, tree)) {
                // New node created
                node->right = balance(node->right);
                return 1;
            }
 
            return 0;
        }

        // Create new node as right child
        node->right = node_create(key, NULL, key_size, 0); // Create new node
        set_val(&node->right->val, &node->right->val_size); // Allow caller to set val

        //Increment tree height if necessary
        if(node->height == tree->max_height)
            tree->max_height++;

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
        tree->size++;
        
        return 0;
    }

    
    if(node_set(tree->root, key, key_size, set_val, tree)) {
        tree->size++; // Update size
        tree->root = balance(tree->root); // Ensure root is balanced
        tree->max_height = tree->root ? tree->root->height : 0; // Set node height

        return 0;
    }
    

    return 1;
}


uint32_t tree_size(Tree* tree) {
    return tree->size;
}

#ifdef TEST
void node_print_level(Node* node, void (*print)(const void*, const void*, const size_t, const size_t), int level) {
    if(!node) // Node null
        return;

    node_print_level(node->left, print, level + 1); // Search left


    print(node->key, node->val, node->key_size, node->val_size); // Print val using passed function
    printf("(Level: %d, Height: %d)\n\n", level, node->height);

    node_print_level(node->right, print, level + 1); // Search right
}

void tree_print_level(Tree* tree, void (*print)(const void*, const void*, const size_t, const size_t)) {
    if(!tree || !print)
        return; // Inputs null
    
    node_print_level(tree->root, print, 0); // Print tree
}
#endif


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

void node_free(Node* node) {
    if(node == NULL) // Ensure node is not null 
        return;

    // Recursively free children
    node_free(node->left);
    node_free(node->right);

    // Deallocate memory for key and value
    if(node->key)
        free(node->key);
    if(node->val)
        free(node->val);

    // Deallocate node struct
    free(node);
}

void tree_free(Tree* tree) {
    if(!tree) // Ensure tree is not null
        return;

    if(tree->root) // Deallocate all nodes
        node_free(tree->root);
    
    // Deallocate tree
    free(tree);
}


void tree_iter_grow_stack(TreeIter* tree_iter) {
    if(!tree_iter) // Ensure non-null input
        return; 

    uint8_t new_capacity; // Nex capacity for iterator stack

    // Calculate new capacity
    if((uint16_t)tree_iter->stack_capacity + (tree_iter->stack_capacity / 2) > UINT8_MAX) // Check for possible overflow
        new_capacity = UINT8_MAX; // Set stack to max size
    else // Double current stack size
        new_capacity = tree_iter->stack_capacity + (tree_iter->stack_capacity / 2);

    // Allocate memory for new stack
    Node** new_stack = malloc(new_capacity * sizeof(Node*));

    if(!new_stack) // Handle allocation failure
        return;

    // Copy old stack to new and free
    memcpy(new_stack, tree_iter->node_stack, tree_iter->stack_size * sizeof(Node*));
    free(tree_iter->node_stack);  

    // Update iterator fields
    tree_iter->node_stack = new_stack;
    tree_iter->stack_capacity = new_capacity;
}

void tree_iter_push_left(TreeIter* tree_iter, Node* node) {
    Node* temp = node; // Traversal node

    while(temp) { // Iterate through node and all left children
        // Grow stack if necessary
        if(tree_iter->stack_size == tree_iter->stack_capacity)
            tree_iter_grow_stack(tree_iter);

        // Push to stack
        tree_iter->node_stack[tree_iter->stack_size++] = temp;
        temp = temp->left;
    }
}

TreeIter* tree_iter_create(Tree* tree) {
    if(!tree) // Ensure non-null input
        return NULL;

    // Allocate memory for iterator,
    TreeIter* tree_iter = malloc(sizeof(TreeIter));

    if(!tree_iter) // Handle allocation failure
        return NULL;

    // Allocate memory for node stack
    tree_iter->node_stack = malloc(tree->max_height * sizeof(Node*));

    if(!tree_iter->node_stack) { // Handle allocation failure
        free(tree_iter); // Deallocate iterator memory
        return NULL;
    }

    // Initialize stack size and capacity
    tree_iter->stack_size = 0;
    tree_iter->stack_capacity = tree->max_height;

    // Push leftmost nodes to stack
    tree_iter_push_left(tree_iter, tree->root);

    return tree_iter;
}

char tree_iter_has_next(TreeIter* tree_iter) {
    if(!tree_iter) // Ensure non-null input
        return 0;

    if(tree_iter->stack_size < 1) // Check if stack is empty
        return 0; // No nodes left

    return 1; // Next node available
}


char tree_iter_next(TreeIter* tree_iter, void** key, size_t* key_size, void** val, size_t* val_size) {
    // Ensure non-null inputs
    if(!tree_iter || !key || !val)
        return 0;
    
    if(tree_iter->stack_size < 1)
        return 0;

    // Get next node
    Node* next = tree_iter->node_stack[--tree_iter->stack_size];

    // Set pointer arguments to node's key and value
    *key = next->key;
    *val = next->val;

    // Set key and val size if not null
    if(key_size)
        *key_size = next->key_size;
    if(val_size)
        *val_size = next->val_size;

    // Push right child and it's leftmost children to stack
    if(next->right)
        tree_iter_push_left(tree_iter, next->right);

    return 1;
}


void tree_iter_free(TreeIter* tree_iter) {
    if(!tree_iter) // Ensure non-null input
        return;

    // Deallocate stack memory
    if(tree_iter->node_stack)
        free(tree_iter->node_stack);

    // Deallocate iterator memory
    free(tree_iter);
}

