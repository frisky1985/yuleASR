/*
 * libc_stubs.c - minimal freestanding libc stubs for -nostdlib build
 */
#include <stddef.h>

void * memset( void * dest, int c, size_t n )
{
    unsigned char * d = ( unsigned char * ) dest;

    while ( n > 0 )
    {
        *d = ( unsigned char ) c;
        d++;
        n--;
    }

    return dest;
}

void * memcpy( void * dest, const void * src, size_t n )
{
    unsigned char *       d = ( unsigned char * ) dest;
    const unsigned char * s = ( const unsigned char * ) src;

    while ( n > 0 )
    {
        *d = *s;
        d++;
        s++;
        n--;
    }

    return dest;
}

void * memmove( void * dest, const void * src, size_t n )
{
    unsigned char *       d = ( unsigned char * ) dest;
    const unsigned char * s = ( const unsigned char * ) src;

    if ( d < s )
    {
        while ( n > 0 )
        {
            *d = *s;
            d++;
            s++;
            n--;
        }
    }
    else
    {
        d += n;
        s += n;
        while ( n > 0 )
        {
            d--;
            s--;
            *d = *s;
            n--;
        }
    }

    return dest;
}

int memcmp( const void * s1, const void * s2, size_t n )
{
    const unsigned char * p1 = ( const unsigned char * ) s1;
    const unsigned char * p2 = ( const unsigned char * ) s2;

    while ( n > 0 )
    {
        if ( *p1 != *p2 )
        {
            return ( *p1 < *p2 ) ? -1 : 1;
        }
        p1++;
        p2++;
        n--;
    }

    return 0;
}
