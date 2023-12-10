#include "transaction.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static unsigned int global_transaction_id = 1;

// Initialize the transaction manager
void trans_init(void) {
    // Initialize the global transaction list and other necessary structures
    pthread_mutex_init(&trans_list.mutex, NULL);
    trans_list.next = &trans_list;
    trans_list.prev = &trans_list;
}

// Finalize the transaction manager
void trans_fini(void) {
    // Clean up the global transaction list and other structures
    pthread_mutex_destroy(&trans_list.mutex);
}

// Create a new transaction
TRANSACTION *trans_create(void) {
    TRANSACTION *tp = malloc(sizeof(TRANSACTION));
    if (tp == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&trans_list.mutex);
    tp->id = global_transaction_id++;
    pthread_mutex_unlock(&trans_list.mutex);

    // Initialize transaction fields
    tp->refcnt = 1;
    tp->status = TRANS_PENDING;
    tp->depends = NULL;
    tp->waitcnt = 0;
    sem_init(&tp->sem, 0, 0);
    pthread_mutex_init(&tp->mutex, NULL);

    // Add to the global transaction list
    pthread_mutex_lock(&trans_list.mutex);
    tp->next = trans_list.next;
    tp->prev = &trans_list;
    trans_list.next->prev = tp;
    trans_list.next = tp;
    pthread_mutex_unlock(&trans_list.mutex);

    return tp;
}

// Increase the reference count on a transaction
TRANSACTION *trans_ref(TRANSACTION *tp, char *why) {
    pthread_mutex_lock(&tp->mutex);
    tp->refcnt++;
    pthread_mutex_unlock(&tp->mutex);
    return tp;
}

// Decrease the reference count on a transaction
void trans_unref(TRANSACTION *tp, char *why) {
    pthread_mutex_lock(&tp->mutex);
    if (--tp->refcnt == 0) {
        // Remove from the global transaction list
        pthread_mutex_lock(&trans_list.mutex);
        tp->prev->next = tp->next;
        tp->next->prev = tp->prev;
        pthread_mutex_unlock(&trans_list.mutex);

        // Clean up
        pthread_mutex_unlock(&tp->mutex);
        pthread_mutex_destroy(&tp->mutex);
        sem_destroy(&tp->sem);
        free(tp);
    } else {
        pthread_mutex_unlock(&tp->mutex);
    }
}

// Add a transaction to the dependency set for this transaction
void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp) {
    DEPENDENCY *dep = malloc(sizeof(DEPENDENCY));
    if (dep == NULL) {
        return;
    }

    dep->trans = dtp;
    pthread_mutex_lock(&tp->mutex);
    dep->next = tp->depends;
    tp->depends = dep;
    pthread_mutex_unlock(&tp->mutex);
}

// Try to commit a transaction
TRANS_STATUS trans_commit(TRANSACTION *tp) {
    pthread_mutex_lock(&tp->mutex);
    if (tp->status != TRANS_PENDING) {
        pthread_mutex_unlock(&tp->mutex);
        return tp->status;
    }

    // Check dependencies
    DEPENDENCY *dep = tp->depends;
    while (dep != NULL) {
        pthread_mutex_lock(&dep->trans->mutex);
        if (dep->trans->status == TRANS_ABORTED) {
            tp->status = TRANS_ABORTED;
            pthread_mutex_unlock(&dep->trans->mutex);
            pthread_mutex_unlock(&tp->mutex);
            return TRANS_ABORTED;
        }
        pthread_mutex_unlock(&dep->trans->mutex);
        dep = dep->next;
    }

    tp->status = TRANS_COMMITTED;
    pthread_mutex_unlock(&tp->mutex);
    return TRANS_COMMITTED;
}

// Abort a transaction
TRANS_STATUS trans_abort(TRANSACTION *tp) {
    pthread_mutex_lock(&tp->mutex);
    if (tp->status == TRANS_COMMITTED) {
        fprintf(stderr, "Fatal error: Attempt to abort a committed transaction\n");
        exit(EXIT_FAILURE);
    }

    tp->status = TRANS_ABORTED;
    pthread_mutex_unlock(&tp->mutex);
    return TRANS_ABORTED;
}

// Get the current status of a transaction
TRANS_STATUS trans_get_status(TRANSACTION *tp) {
    TRANS_STATUS status;
    pthread_mutex_lock(&tp->mutex);
    status = tp->status;
    pthread_mutex_unlock(&tp->mutex);
    return status;
}

// Debugging functions
void trans_show(TRANSACTION *tp) {
    // Implement transaction display logic
}

void trans_show_all(void) {
    // Implement logic to display all transactions
}
