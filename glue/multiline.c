#include <glib.h>
#include <string.h>
#include "constants.h"

/*
 * Fills out_pieces with owned gchar* (caller frees each, then g_array_free(pieces, FALSE)).
 * off: one piece (original text).
 * pipe: one piece (newlines replaced by |).
 * multimessage: one piece per line (CRLF/CR/LF normalized to line breaks).
 */
void gowhatsapp_multiline_apply(const char *option_value, const char *text, GArray *out_pieces)
{
    if (!text)
        text = "";
    if (!option_value || strcmp(option_value, GOWHATSAPP_MULTILINE_CONVERSION_CHOICE_OFF) == 0) {
        gchar *dup = g_strdup(text);
        g_array_append_val(out_pieces, dup);
        return;
    }
    /* Normalize: \r\n and \r -> \n */
    GString *norm = g_string_sized_new((guint)strlen(text));
    for (const char *p = text; *p; p++) {
        if (p[0] == '\r' && p[1] == '\n') {
            g_string_append_c(norm, '\n');
            p++;
            continue;
        }
        if (p[0] == '\r') {
            g_string_append_c(norm, '\n');
            continue;
        }
        g_string_append_c(norm, *p);
    }
    const char *n = norm->str;

    if (strcmp(option_value, GOWHATSAPP_MULTILINE_CONVERSION_CHOICE_PIPE) == 0) {
        gchar *pipe_str = g_strdup(n);
        for (char *q = pipe_str; *q; q++)
            if (*q == '\n')
                *q = '|';
        g_array_append_val(out_pieces, pipe_str);
        g_string_free(norm, TRUE);
        return;
    }
    if (strcmp(option_value, GOWHATSAPP_MULTILINE_CONVERSION_CHOICE_MULTIMESSAGE) == 0) {
        gchar **lines = g_strsplit(n, "\n", -1);
        for (gchar **line = lines; *line; line++) {
            gchar *dup = g_strdup(*line);
            g_array_append_val(out_pieces, dup);
        }
        g_strfreev(lines);
        g_string_free(norm, TRUE);
        return;
    }
    {
        gchar *dup = g_strdup(text);
        g_array_append_val(out_pieces, dup);
    }
    g_string_free(norm, TRUE);
}
