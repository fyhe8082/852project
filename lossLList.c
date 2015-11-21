#include<stdlib.h>

struct node {
    uint32_t seqNum;
    uint64_t timeArrived;
    bool isNewLoss;
    struct node * next;
} node_t;


int pop(node_t ** head) {
    int retval = -1;
    node_t * next_node = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_node = (*head)->next;
    retval = (*head)->seqNum;
    free(*head);
    *head = next_node;

    return retval;
}

int remove_by_seqNum(node_t ** head, int seqNum) {
    int i = 0;
    int retval = -1;
    node_t * current = *head;
    node_t * temp_node = NULL;


    if ((*head)->seqNum == seqNum) {
        return pop(head);
    }

    while (current->next->seqNUm != seqNum) {
        if (current->next == NULL) {
            return -1;
        }
        current = current->next;
    }

    temp_node = current->next;
    //if the deleted one is loss start, charge the next one to be loss start
    if (temp_node->isNewLoss==1)
        temp_node->next->isNewLoss = 1;

    retval = temp_node->seqNum;
    current->next = temp_node->next;
    free(temp_node);

    return retval;

}

void append(node_t ** head, int seqNum, uint64_t timeArrived) {
    node_t * new_node;
    node_t * p = *head;
    new_node = malloc(sizeof(node_t));

    new_node->seqNum = seqNum;
    new_node->timeArrived = timeArrived;
    while(p->next != NULL)p=p->next;
    new_node->next = NULL;
    p->next = new_node;
}
