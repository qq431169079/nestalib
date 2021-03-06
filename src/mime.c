/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * The MIT License
 *
 * Copyright (c) 2008-2019 YAMAMOTO Naoki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define API_INTERNAL
#include "nestalib.h"

/*
 * MIME エンコードされた文字列を作成します。
 *      MIME: Multipurpose Internet Mail Extension
 *
 * 文字コードは ISO-2022-JP(JIS) を使用します。
 * エンコードは base64 を使用します。
 *
 * エンコーディングされた文字列が dst に格納されます。
 * dst には十分な領域が必要になります。
 *
 * dst:     MIMEエンコードされた文字列を格納する領域
 * dstsize: MIMEエンコードされた文字列を格納する領域サイズ
 * src:     変換する文字列
 * srcenc:  変換する文字列のエンコード名
 *
 * 戻り値
 *  dstのポインタを返します。
 *  文字コード変換でエラーの場合は NULL を返します。
 */
APIEXPORT char* mime_encode(char* dst, int dst_size, const char* src, const char* src_enc)
{
    size_t buf_size;
    char* jis_buf;
    char* base64_buf;
    const char* jisenc = "ISO-2022-JP";

    buf_size = strlen(src) * 2;
    jis_buf = (char*)alloca(buf_size);
    if (convert(src_enc, src, (int)strlen(src), jisenc, jis_buf, (int)buf_size) < 0) {
        err_write("mime_encode: convert error. %s(%s)", src, src_enc);
        return NULL;
    }
    base64_buf = (char*)alloca(strlen(jis_buf) * 2);
    base64_encode(base64_buf, jis_buf, (int)strlen(jis_buf));
    snprintf(dst, dst_size, "=?%s?B?%s?=", jisenc, base64_buf);
    return dst;
}

/*
 * MIME エンコードされた文字列を元に戻します。
 *      MIME: Multipurpose Internet Mail Extension
 *
 * MIMEエンコードされた文字列を指定されたエンコードで元に戻します。
 *
 * dst:      文字列を格納する領域
 * dstsize:  文字列を格納する領域サイズ
 * dst_enc:  文字列dstのエンコード名
 * mime_str: MIMEエンコードされた文字列
 *
 * 戻り値
 *  dstのポインタを返します。
 *  エラーの場合は NULL を返します。
 */
APIEXPORT char* mime_decode(char* dst, int dst_size, const char* dst_enc, const char* mime_str)
{
    int nq;
    size_t len;
    char* str;
    char** listp;
    char* m_enc = NULL;
    char* m_type = NULL;
    char* m_str = NULL;
    char* dec_buf;
    int dec_len;

    nq = strchc(mime_str, '?');
    if (nq != 4) {
        err_write("mime_decode: invalid mime string format, %s", mime_str);
        return NULL;
    }

    len = strlen(mime_str);
    if (strncmp(mime_str, "=?", 2) != 0 ||
        strncmp(mime_str+len-2, "?=", 2) != 0) {
        err_write("mime_decode: invalid mime string '=?...?=', %s", mime_str);
        return NULL;
    }

    str = (char*)alloca(len+1);
    strcpy(str, mime_str);
    listp = split(str, '?');
    if (listp != NULL) {
        /* listp[0] is '=' */
        if (listp[1] != NULL) {
            m_enc = listp[1];
            if (listp[2] != NULL) {
                m_type = listp[2];
                if (listp[3] != NULL)
                    m_str = listp[3];
            }
        }
    }
    if (m_enc == NULL || m_type == NULL || m_str == NULL) {
        err_write("mime_decode: invalid mime format, %s", mime_str);
        list_free(listp);
        return NULL;
    }

    if (*m_type != 'B') {
        err_write("mime_decode: invalid base64 type, %s", mime_str);
        list_free(listp);
        return NULL;
    }

    /* base64 でデコードします。*/
    dec_buf = (char*)alloca(len);
    dec_len = base64_decode(dec_buf, m_str);
    dec_buf[dec_len] = '\0';

    /* 文字コード変換を行います(m_enc -> dst_enc) */
    if (convert(m_enc, dec_buf, dec_len, dst_enc, dst, dst_size) < 0) {
        err_write("mime_decode: convert error. %s(%s)", m_str, m_enc);
        list_free(listp);
        return NULL;
    }

    list_free(listp);
    return dst;
}
