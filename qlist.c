#include "qlist.h"




#define QLIST_TOKEN_MASK    (0xFF000000)



typedef struct _qitem_t
{
    struct _qitem_t* prev;
    struct _qitem_t* next;
    quint32          token;
    void*            data;
}qitem_t;

typedef struct
{
    qitem_t  items[0];
}qbucket_t;

typedef struct
{
    qint32      buckets_total;
    qint32      buckets_alloc;
    qbucket_t** buckets;

    qint32      free_count;
    qitem_t     free_head;

    qitem_t     used_head;

    qint32      unit_size;
}qlist_t;




static  qbucket_t*  qlist_inc(qlist_t* q)
{
    //  检查下是否到了容量上限
    if (q->buckets_alloc >= q->buckets_total)
    {
        return NULL;
    }

    qbucket_t* bucket = (qbucket_t*)malloc(sizeof(qbucket_t) + sizeof(qitem_t) * q->unit_size);
    if (NULL == bucket)
    {
        return NULL;
    }

    qint32 bucket_index = q->buckets_alloc;
    for (qint32 i = 0; i < q->unit_size; i++)
    {
        qitem_t* item = bucket->items + i;
        item.data   = NULL;
        item->token = (bucket_index * q->unit_size) + i;

        item->next = &(q->free_head);
        item->prev = q->free_head.prev;
        q->free_head.prev->next = item;
        q->free_head.prev = item;
        q->free_count++;
    }

    q->buckets[q->buckets_alloc++] = bucket;
    return bucket;
}




QLIST_EXTERN    qlist   qlist_new(qint32 init_cap, qint32 limit_cap, qint32 unit_size)
{
    //  对所有的参数进行校正和补齐(最小基本单位为64)
    unit_size = ((unit_size <= 0)?64:unit_size);
    unit_size = (unit_size + ((0 != (unit_size % 64))?1:0)) * 64;
    init_cap  = ((init_cap <= 0)?unit_size:init_cap);
    init_cap  = (init_cap + ((0 != (init_cap % unit_size))?1:0)) * unit_size;
    limit_cap = ((limit_cap <= 0)?unit_size:limit_cap);
    limit_cap = (limit_cap + ((0 != (limit_cap % unit_size))?1:0)) * unit_size;
    limit_cap = ((limit_cap < init_cap)?init_cap:limit_cap);

    qlist_t* q = malloc(sizeof(qlist_t));
    if (NULL == q)
    {
        return NULL;
    }
    memset(q, 0, sizeof(qlist_t));


    //  计算桶的总数量
    qint32  buckets_total = limit_cap / unit_size;
    qint32  buckets_init  = init_cap  / unit_size;


    //  分配桶的数组
    qbucket_t** bucktes = (qbucket_t**)malloc(sizeof(qbucket_t*) * buckets_total);
    if (NULL == bucktes)
    {
        free(bucktes);
        return NULL;
    }


    //  填充 qlist 的初步数据
    q->buckets_total   = buckets_total;
    q->buckets_alloc   = 0;
    q->buckets         = bucktes;
    q->free_count      = 0;
    q->free_head.data  = NULL;
    q->free_head.next  = &(q->free_head);
    q->free_head.prev  = &(q->free_head);
    q->free_head.token = QLIST_INVALID_TOKEN;
    q->used_head.data  = NULL;
    q->used_head.next  = &(q->used_head);
    q->used_head.prev  = &(q->used_head);
    q->used_head.token = QLIST_INVALID_TOKEN;
    q->unit_size = unit_size;

    //  逐个桶分配
    for (qint32 bucket_index = 0; bucket_index < buckets_init; bucket_index)
    {
        if (NULL == qlist_inc(q))
        {
            break;
        }
    }
}




QLIST_EXTERN    void    qlist_del(qlist pq)
{
    qlist_t* q = (qlist_t*)pq;
    if (NULL == pq)
    {
        return;
    }

    for (qint32 i = 0; i < q->buckets_alloc; i++)
    {
        free(q->buckets[i]);
    }

    free(q->buckets);

    free(pq);
}



QLIST_EXTERN    quint32 qlist_save(qlist pq, void* data)
{
    if (NULL == data)
    {
        return QLIST_INVALID_TOKEN;
    }

    qlist_t* q = (qlist_t*)pq;

    qitem_t* item = q->free_head.next;
    if (item == &(q->free_head))
    {
        if (NULL == qlist_inc(q))
        {
            return QLIST_INVALID_TOKEN;
        }

        item = q->free_head.next;
    }

    //  将 item 从 free 链表里面拆出来
    q->free_head.next = q->free_head.next->next;
    q->free_head.next->prev = &(q->free_head);
    q->free_count--;

    //  将 item 添加到 used 链表
    item->next = &(q->used_head);
    item->prev = q->used_head.prev;
    q->used_head.prev->next = item;
    q->used_head.prev = item;

    //  将用户数据存起来
    item->data = data;

    //  返回 token
    return item->token;
}




static qitem_t* qlist_anchor(qlist_t* q, quint32 token)
{
    //  计算待删除的项所处的位置
    qint32 bucket_index = (token & (~QLIST_TOKEN_MASK)) / q->unit_size;
    qint32 item_index   = (token & (~QLIST_TOKEN_MASK)) % q->unit_size;
    if (bucket_index >= q->buckets_alloc)
    {
        return NULL;
    }

    //  找对应的项
    return q->buckets[bucket_index]->items + item_index;
}




QLIST_EXTERN    void* qlist_remove(qlist pq, quint32 token)
{
    qlist_t* q = (qlist_t*)pq;

    //  定位到对应的 item
    qitem_t* item = qlist_anchor(q, token);
    if (NULL == item)
    {
        return NULL;
    }

    //  检查 token 是否被重用
    if (item->token != token)
    {
        return NULL;
    }

    //  出具先清理掉
    void* data = item->data;
    item->data = NULL;

    //  从 used 列中摘除
    item->prev->next = item->next;
    item->next->prev = item->prev;

    //  添加到 free 队列
    item->next = &(q->free_head);
    item->prev = q->free_head.prev;
    q->free_head.prev->next = item;
    q->free_head.prev = item;
    q->free_count++;

    return data;
}




QLIST_EXTERN    void*   qlist_query(qlist pq, quint32 token)
{
    qlist_t* q = (qlist_t*)pq;

    //  定位到对应的 item
    qitem_t* item = qlist_anchor(q, token);
    if (NULL == item)
    {
        return NULL;
    }

    //  检查 token 是否被重用
    if (item->token != token)
    {
        return NULL;
    }

    return item;
}




QLIST_EXTERN    void    qlist_foreach(qlist pq, int (*callback)(quint32 token, vdoi* data, void* ctx), void* ctx)
{
    qlist_t* q = (qlist_t*)pq;
    qitem_t* itr = q->used_head.next;
    while (itr != &(q->used_head))
    {
        qitem_t* item = itr;
        itr = itr->next;

        if (0 != (*callback)(item->token, item->data, ctx))
        {
            break;
        }
    }
}




