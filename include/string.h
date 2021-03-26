/**
 Makeshift compatibility header
*/
#ifdef __cplusplus
extern "C" {
#endif
    typedef unsigned long long size_t;
    
    void *memcpy(void *dest, const void *src, size_t n);
    
    int strcmp(const char *s1, const char *s2);
    int strncmp(const char *s1, const char *s2, size_t n);
    char *strcpy(char *dest, const char *src);
    char *strncpy(char *dest, const char *src, size_t n);

#ifdef __cplusplus
};
#endif


