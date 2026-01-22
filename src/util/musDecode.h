/* musDecode.h - header-only Minecraft .mus decoder
 *
 * Usage:
 *   #include "musDecode.h"
 *   int rc = mus_decode_file("in.mus", "out.ogg", "in.mus");
 *
 * Notes:
 *   - outname MUST be the exact filename used by the Java encoder output,
 *     because the seed is based on Java's String.hashCode() of that name.
 */

#ifndef MUS_DECODE_H
#define MUS_DECODE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t mus_java_string_hashcode(const char *s)
{
    int32_t h = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        /* Java: h = 31*h + ch; with 32-bit wraparound */
        h = (int32_t)((uint32_t)h * 31u + (uint32_t)(*p));
        ++p;
    }
    return h;
}

/* Decode input file "inpath" into output file "outpath".
 * outname should typically be the input filename (e.g. "cat.mus").
 * Returns 0 on success, nonzero on failure.
 */
static inline int mus_decode_file(const char *inpath, const char *outpath, const char *outname)
{
    FILE *fin = fopen(inpath, "rb");
    if (!fin) return 1;

    FILE *fout = fopen(outpath, "wb");
    if (!fout) { fclose(fin); return 1; }

    int32_t seed = mus_java_string_hashcode(outname);

    uint8_t buff[65536];
    size_t nread;

    while ((nread = fread(buff, 1, sizeof(buff), fin)) > 0) {
        for (size_t i = 0; i < nread; i++) {
            uint8_t enc = buff[i];

            /* Reverse: enc = val ^ (seed >> 8) */
            uint8_t key = (uint8_t)((uint32_t)seed >> 8);
            uint8_t val = (uint8_t)(enc ^ key);

            buff[i] = val;

            /* Java update: seed = seed * 498729871 + 85731 * (byte)val; */
            seed = (int32_t)(
                (uint32_t)seed * 498729871u +
                (uint32_t)(85731 * (int32_t)(int8_t)val)
            );
        }

        fwrite(buff, 1, nread, fout);
    }

    fclose(fin);
    fclose(fout);
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MUS_DECODE_H */
