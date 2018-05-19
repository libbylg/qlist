#ifndef _QLIST_H_
#define _QLIST_H_

#ifndef __cplusplus
#define QLIST_EXTERN    extern  "C"
#else
#define QLIST_EXTERN    extern
#endif




typedef unsigned int    quint32;
typedef int             qint32;
typedef void            qvoid;




typedef qvoid*  qlist;




#define QLIST_INVALID_TOKEN     (0xFFFFFFFF)
#define QLIST_CAP_MAX           (0x00FFFFFF + 1)




QLIST_EXTERN    qlist   qlist_new(qint32 init_cap, qint32 limit_cap, qint32 unit_size);
QLIST_EXTERN    void    qlist_del(qlist q);
QLIST_EXTERN    quint32 qlist_save(qlist q, void* data);
QLIST_EXTERN    quint32 qlist_remove(qlist q, quint32 token);
QLIST_EXTERN    void*   qlist_query(qlist q, quint32 token);
QLIST_EXTERN    void    qlist_foreach(qlist q, int (*callback)(quint32 token, vdoi* data, void* ctx), void* ctx);




#endif//_QLIST_H_
