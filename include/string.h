/**
 Makeshift compatibility header
*/
#ifdef __cplusplus
extern "C" {
#endif
    typedef unsigned long long size_t;
    
    void *memcpy(void *dest, const void *src, size_t n);

#ifdef __cplusplus
};
#endif


