#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_EDGES 256 // Simple approach: one edge per possible starting character

// Forward declaration
typedef struct RadixNode RadixNode;

// Edge structure
typedef struct {
char *label;
RadixNode *target_node;
} RadixEdge;

// Node structure
struct RadixNode {
bool is_end_of_word;
RadixEdge *edges[MAX_EDGES];
int num_edges;
};

// --- Utility Functions ---

RadixNode* create_node(bool is_end) {
RadixNode* node = (RadixNode*)malloc(sizeof(RadixNode));
node->is_end_of_word = is_end;
node->num_edges = 0;
for (int i = 0; i < MAX_EDGES; i++) {
node->edges[i] = NULL;
}
return node;
}

int get_common_prefix_length(const char* s1, const char* s2) {
int len = 0;
while (s1[len] != '\0' && s2[len] != '\0' && s1[len] == s2[len]) {
len++;
}
return len;
}

void add_edge(RadixNode* node, const char* label, RadixNode* target) {
RadixEdge* edge = (RadixEdge*)malloc(sizeof(RadixEdge));
edge->label = strdup(label);
edge->target_node = target;
// We use the first character of the label as a quick index if needed,
// or just store it contiguously. We'll store contiguously for simplicity.
node->edges[node->num_edges++] = edge;
}

void remove_edge(RadixNode* node, int index) {
free(node->edges[index]->label);
free(node->edges[index]);
// Shift remaining edges
for (int i = index; i < node->num_edges - 1; i++) {
node->edges[i] = node->edges[i+1];
}
node->num_edges--;
}

// --- Main Radix Tree Operations ---

bool insert(RadixNode* root, const char* word) {
RadixNode* curr = root;
int word_idx = 0;
int word_len = strlen(word);

while (word_idx < word_len) {
    const char* suffix = word + word_idx;
    RadixEdge* match_edge = NULL;
    int match_edge_idx = -1;

    // Find edge starting with the same character
    for (int i = 0; i < curr->num_edges; i++) {
        if (curr->edges[i]->label[0] == suffix[0]) {
            match_edge = curr->edges[i];
            match_edge_idx = i;
            break;
        }
    }

    if (!match_edge) {
        // No matching edge, create a new one
        RadixNode* new_node = create_node(true);
        add_edge(curr, suffix, new_node);
        return true; // Inserted
    }

    int common_len = get_common_prefix_length(match_edge->label, suffix);

    if (common_len == strlen(match_edge->label)) {
        // Full edge match, continue down the tree
        curr = match_edge->target_node;
        word_idx += common_len;
    } else {
        // Partial match, split the edge
        RadixNode* split_node = create_node(false);
        
        // 1. Move the remainder of the existing edge to a new child of split_node
        const char* remain_edge_label = match_edge->label + common_len;
        add_edge(split_node, remain_edge_label, match_edge->target_node);
        
        // 2. Modify the current edge to point to split_node
        char* new_label = strndup(match_edge->label, common_len);
        free(match_edge->label);
        match_edge->label = new_label;
        match_edge->target_node = split_node;

        // 3. Add the remainder of the new word as another child of split_node (if any)
        const char* remain_word_label = suffix + common_len;
        if (strlen(remain_word_label) > 0) {
            RadixNode* new_leaf = create_node(true);
            add_edge(split_node, remain_word_label, new_leaf);
        } else {
            split_node->is_end_of_word = true;
        }
        return true;
    }
}

if (!curr->is_end_of_word) {
    curr->is_end_of_word = true;
    return true;
}
return false; // Word already exists


}

bool search(RadixNode* root, const char* word) {
RadixNode* curr = root;
int word_idx = 0;
int word_len = strlen(word);

while (word_idx < word_len) {
    const char* suffix = word + word_idx;
    RadixEdge* match_edge = NULL;

    for (int i = 0; i < curr->num_edges; i++) {
        if (curr->edges[i]->label[0] == suffix[0]) {
            match_edge = curr->edges[i];
            break;
        }
    }

    if (!match_edge) return false;

    int common_len = get_common_prefix_length(match_edge->label, suffix);
    
    // If the common prefix doesn't cover the full edge, the word is not here.
    // Exception: If the edge is longer than the remaining word, it's a mismatch.
    if (common_len != strlen(match_edge->label) || common_len > strlen(suffix)) {
        return false;
    }

    curr = match_edge->target_node;
    word_idx += common_len;
}

return curr->is_end_of_word;


}

// Recursive helper for deletion
bool remove_helper(RadixNode* node, const char* word, int depth) {
if (depth == strlen(word)) {
if (!node->is_end_of_word) return false;
node->is_end_of_word = false;
return true;
}

const char* suffix = word + depth;
RadixEdge* match_edge = NULL;
int edge_idx = -1;

for (int i = 0; i < node->num_edges; i++) {
    if (node->edges[i]->label[0] == suffix[0]) {
        match_edge = node->edges[i];
        edge_idx = i;
        break;
    }
}

if (!match_edge) return false;

int common_len = get_common_prefix_length(match_edge->label, suffix);
if (common_len != strlen(match_edge->label)) return false;

bool deleted = remove_helper(match_edge->target_node, word, depth + common_len);

if (deleted) {
    RadixNode* child = match_edge->target_node;
    
    // If child has 0 edges and is not end of word, prune the branch
    if (child->num_edges == 0 && !child->is_end_of_word) {
        remove_edge(node, edge_idx);
        free(child);
    }
    // If child has exactly 1 edge and is not end of word, merge nodes
    else if (child->num_edges == 1 && !child->is_end_of_word) {
        RadixEdge* single_edge = child->edges[0];
        
        // Concatenate labels
        char* merged_label = malloc(strlen(match_edge->label) + strlen(single_edge->label) + 1);
        strcpy(merged_label, match_edge->label);
        strcat(merged_label, single_edge->label);
        
        free(match_edge->label);
        match_edge->label = merged_label;
        match_edge->target_node = single_edge->target_node;
        
        // Cleanup old child
        free(single_edge->label);
        free(single_edge);
        free(child);
    }
}

return deleted;


}

bool delete_word(RadixNode* root, const char* word) {
return remove_helper(root, word, 0);
}

// --- Test Implementation ---
int main() {
RadixNode* tree = create_node(false);

printf("Inserting 'radix', 'radiacao', 'arvore'\n");
insert(tree, "radix");
insert(tree, "radiacao");
insert(tree, "arvore");

printf("\nSearching:\n");
printf("Search 'radix': %s\n", search(tree, "radix") ? "Found" : "Not Found");
printf("Search 'rad': %s\n", search(tree, "rad") ? "Found" : "Not Found");
printf("Search 'arvore': %s\n", search(tree, "arvore") ? "Found" : "Not Found");

printf("\nDeleting 'radix'...\n");
delete_word(tree, "radix");

printf("Search 'radix' after deletion: %s\n", search(tree, "radix") ? "Found" : "Not Found");
printf("Search 'radiacao' remains: %s\n", search(tree, "radiacao") ? "Found" : "Not Found");

return 0;


}