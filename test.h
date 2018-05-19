#ifndef __test_H_
#define __test_H_


#ifndef __cplusplus
#define TEST_EXTERN extern "C"
#else
#define TEXT_EXTERN extern
#endif


typedef void (*TEST_PROXY_FUNC)();

typedef struct
{
    
}result_t;

typedef struct _test_t
{
    struct _test_t* next;
    struct _test_t* prev;
    TEST_PROXY_FUNC proxy;
}test_t;


#define TEST(name)  \
    extern  void test_##name(); \
    void test_##name()

#endif//__test_H_


