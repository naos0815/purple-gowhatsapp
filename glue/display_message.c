#include "gowhatsapp.h"
#include "constants.h"

void gowhatsapp_display_text_message(
    PurpleAccount *account, 
    const gchar * senderJid,
    const gchar * remoteJid,
    const gchar * text,
    const time_t timestamp,
    const gboolean isGroup,
    const gboolean isOutgoing,
    const gchar * name,
    PurpleMessageFlags flags,
    const gchar * messageId,
    const gboolean escape
) {
    g_return_if_fail(account != NULL);
    
    PurpleConnection * connection = purple_account_get_connection(account);
    
    if (flags & PURPLE_MESSAGE_SYSTEM) {
        if (senderJid == NULL) {
            senderJid = g_strdup("system"); // g_strdup needed since senderJid is freed by caller
        }
        gboolean bridge = purple_account_get_bool(account, GOWHATSAPP_BRIDGE_COMPATIBILITY_OPTION, FALSE);
        if (bridge) {
            // spectrum ignores system messages: strip the system flag
            flags &= ~PURPLE_MESSAGE_SYSTEM;
        } else {
            // normal Procedure: keep system flag, do not log message
            flags |= PURPLE_MESSAGE_NO_LOG;
        }
    }

    if (purple_strequal(purple_account_get_username(account), senderJid)) {
        flags |= PURPLE_MESSAGE_SEND;
        // Note: For outgoing messages (no matter if local echo or sent by other device),
        // PURPLE_MESSAGE_SEND must be set due to how purple_conversation_write is implemented
        if (!isOutgoing) {
            // special handling of messages sent by self incoming from remote, addressing issue #32
            // adjusted for Spectrum, see issue #130
            flags |= PURPLE_MESSAGE_REMOTE_SEND;
        }
    } else {
        flags |= PURPLE_MESSAGE_RECV;
    }

    const char *multiline_opt = purple_account_get_string(account, GOWHATSAPP_MULTILINE_CONVERSION_OPTION, GOWHATSAPP_MULTILINE_CONVERSION_CHOICE_OFF);
    GArray *pieces = g_array_new(FALSE, FALSE, sizeof(gchar *));
    gowhatsapp_multiline_apply(multiline_opt, text, pieces);

    for (guint i = 0; i < pieces->len; i++) {
        gchar *piece = g_array_index(pieces, gchar *, i);

        // WhatsApp is a plain-text protocol, but Pidgin expects HTML
        gchar * escaped_text = NULL;
        if (escape) {
            gchar * html = purple_markup_escape_text(piece, -1);
            escaped_text = purple_strdup_withhtml(html);
            g_free(html);
        } else {
            escaped_text = g_strdup(piece);
        }

        gchar * text_with_id = NULL;
        if (purple_account_get_bool(account, GOWHATSAPP_DISPLAY_MESSAGE_ID_OPTION, FALSE) && messageId != NULL && i == 0) {
            text_with_id = g_strdup_printf("%s <span lang=\"id\">%s</span>", escaped_text, messageId);
        } else {
            text_with_id = g_strdup(escaped_text);
        }

        g_free(escaped_text);
        
        if (isGroup) {
            gowhatsapp_enter_group_chat(connection, remoteJid, NULL);
            purple_serv_got_chat_in(connection, g_str_hash(remoteJid), senderJid, flags, text_with_id, timestamp);
        } else {
            if (flags & PURPLE_MESSAGE_SEND) {
                PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, remoteJid, account);
                if (conv == NULL) {
                    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, remoteJid);
                }
                purple_conv_im_write(purple_conversation_get_im_data(conv), remoteJid, text_with_id, flags, timestamp);
            } else {
                if (purple_account_get_bool(account, GOWHATSAPP_UPDATE_BUDDY_ON_MESSAGE_OPTION, TRUE)) {
                    gowhatsapp_ensure_buddy_in_blist(account, remoteJid, name);
                }
                purple_serv_got_im(connection, remoteJid, text_with_id, flags, timestamp);
            }
        }
        
        g_free(text_with_id);
        g_free(piece);
    }
    g_array_free(pieces, FALSE);
}
