#include "data.h"
#include <string.h>

BLOB *blob_create(char *content, size_t size) {
    if (content == NULL || size == 0) return NULL;

    BLOB *blob = malloc(sizeof(BLOB));
    if (!blob) return NULL;

    blob->content = malloc(size);
    if (!blob->content) {
        free(blob);
        return NULL;
    }

    memcpy(blob->content, content, size);
    blob->size = size;
    blob->refcnt = 1;
    pthread_mutex_init(&blob->mutex, NULL);

    return blob;
}

BLOB *blob_ref(BLOB *bp, char *why) {
    if (!bp) return NULL;

    pthread_mutex_lock(&bp->mutex);
    bp->refcnt++;
    pthread_mutex_unlock(&bp->mutex);

    return bp;
}

void blob_unref(BLOB *bp, char *why) {
    if (!bp) return;

    pthread_mutex_lock(&bp->mutex);
    bp->refcnt--;
    if (bp->refcnt <= 0) {
        pthread_mutex_unlock(&bp->mutex);
        pthread_mutex_destroy(&bp->mutex);
        free(bp->content);
        free(bp);
    } else {
        pthread_mutex_unlock(&bp->mutex);
    }
}

int blob_compare(BLOB *bp1, BLOB *bp2) {
    if (!bp1 || !bp2) return -1;
    if (bp1->size != bp2->size) return -1;
    return memcmp(bp1->content, bp2->content, bp1->size);
}

int blob_hash(BLOB *bp) {
    if (!bp || !bp->content) return 0;

    int hash = 0;
    for (size_t i = 0; i < bp->size; i++) {
        hash = 31 * hash + bp->content[i];
    }
    return hash;
}

KEY *key_create(BLOB *bp) {
    if (!bp) return NULL;

    KEY *key = malloc(sizeof(KEY));
    if (!key) return NULL;

    key->blob = bp;
    key->hash = blob_hash(bp);

    return key;
}

void key_dispose(KEY *kp) {
    if (!kp) return;

    blob_unref(kp->blob, "Disposing key blob");
    free(kp);
}

int key_compare(KEY *kp1, KEY *kp2) {
    if (!kp1 || !kp2) return -1;
    return blob_compare(kp1->blob, kp2->blob);
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp) {
    if (!tp || !bp) return NULL;

    VERSION *version = malloc(sizeof(VERSION));
    if (!version) return NULL;

    version->creator = tp;
    version->blob = bp;
    version->next = version->prev = NULL;

    return version;
}

void version_dispose(VERSION *vp) {
    if (!vp) return;

    blob_unref(vp->blob, "Disposing version blob");
    free(vp);
}
