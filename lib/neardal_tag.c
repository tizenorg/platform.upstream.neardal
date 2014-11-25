/*
 *     NEARDAL (Neard Abstraction Library)
 *
 *     Copyright 2012-2014 Intel Corporation. All rights reserved.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License version 2
 *     as published by the Free Software Foundation.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software Foundation,
 *     Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "neard_tag_proxy.h"

#include "neardal.h"
#include "neardal_prv.h"

/*****************************************************************************
 * neardal_tag_prv_cb_property_changed: Callback called when a NFC tag
 * property is changed
 ****************************************************************************/
static void neardal_tag_prv_cb_property_changed(OrgNeardTag *proxy,
						const gchar *arg_unnamed_arg0,
						GVariant *arg_unnamed_arg1,
						void		*user_data)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	TagProp		*tagProp	= user_data;

	(void) proxy; /* remove warning */
	(void) arg_unnamed_arg1; /* remove warning */

	NEARDAL_TRACEIN();

	if (tagProp == NULL || arg_unnamed_arg0 == NULL)
		return;

	NEARDAL_TRACEF("str0='%s'\n", arg_unnamed_arg0);
	NEARDAL_TRACEF("arg_unnamed_arg1=%s (%s)\n",
		       g_variant_print (arg_unnamed_arg1, TRUE),
		       g_variant_get_type_string(arg_unnamed_arg1));


	if (err != NEARDAL_SUCCESS) {
		NEARDAL_TRACE_ERR("Exit with error code %d:%s\n", err,
				neardal_error_get_text(err));
	}

	return;
}

/*****************************************************************************
 * neardal_tag_prv_read_properties: Get Neard Tag Properties
 ****************************************************************************/
static errorCode_t neardal_tag_prv_read_properties(TagProp *tagProp)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	GVariant	*tmp		= NULL;
	GVariant	*tmpOut		= NULL;
	gsize		len;

	NEARDAL_TRACEIN();
	NEARDAL_ASSERT_RET(tagProp != NULL, NEARDAL_ERROR_INVALID_PARAMETER);
	NEARDAL_ASSERT_RET(tagProp->proxy != NULL
			  , NEARDAL_ERROR_GENERAL_ERROR);

	tmp = g_datalist_get_data(&(neardalMgr.dbus_data), tagProp->name);
	if (tmp == NULL) {
		err = NEARDAL_ERROR_NO_TAG;
		NEARDAL_TRACE_ERR("Unable to read tag's properties\n");
		goto exit;
	}
	NEARDAL_TRACEF("Reading:\n%s\n", g_variant_print(tmp, TRUE));

	tmpOut = g_variant_lookup_value(tmp, "TagType", G_VARIANT_TYPE_ARRAY);
	if (tmpOut != NULL) {
		tagProp->tagType = g_variant_dup_strv(tmpOut, &len);
		tagProp->tagTypeLen = len;
		if (len == 0) {
			g_strfreev(tagProp->tagType);
			tagProp->tagType = NULL;
		}
	}

	tmpOut = g_variant_lookup_value(tmp, "Type", G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		tagProp->type = g_variant_dup_string(tmpOut, NULL);

	tmpOut = g_variant_lookup_value(tmp, "ReadOnly",
					G_VARIANT_TYPE_BOOLEAN);
	if (tmpOut != NULL)
		tagProp->readOnly = g_variant_get_boolean(tmpOut);

exit:
	return err;
}

/*****************************************************************************
 * neardal_tag_init: Get Neard Manager Properties = NFC Tags list.
 * Create a DBus proxy for the first one NFC tag if present
 * Register Neard Manager signals ('PropertyChanged')
 ****************************************************************************/
static errorCode_t neardal_tag_prv_init(TagProp *tagProp)
{
	errorCode_t	err = NEARDAL_SUCCESS;

	NEARDAL_TRACEIN();
	NEARDAL_ASSERT_RET(tagProp != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	if (tagProp->proxy != NULL) {
		g_signal_handlers_disconnect_by_func(tagProp->proxy,
			NEARDAL_G_CALLBACK(neardal_tag_prv_cb_property_changed),
						     NULL);
		g_object_unref(tagProp->proxy);
		tagProp->proxy = NULL;
	}

	tagProp->proxy = org_neard_tag_proxy_new_sync(neardalMgr.conn,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
							NEARD_DBUS_SERVICE,
							tagProp->name,
							NULL, /* GCancellable */
							&neardalMgr.gerror);
	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"Unable to create Neard Tag Proxy (%d:%s)\n",
				  neardalMgr.gerror->code,
				  neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		return NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY;
	}

	/* Populate Tag datas... */
	err = neardal_tag_prv_read_properties(tagProp);
	if (err != NEARDAL_SUCCESS)
		return err;

	return err;
}

/*****************************************************************************
 * neardal_tag_prv_free: unref DBus proxy, disconnect Neard Tag signals
 ****************************************************************************/
static void neardal_tag_prv_free(TagProp **tagProp)
{
	NEARDAL_TRACEIN();
	if ((*tagProp)->proxy != NULL) {
		g_signal_handlers_disconnect_by_func((*tagProp)->proxy,
			NEARDAL_G_CALLBACK(neardal_tag_prv_cb_property_changed),
						     NULL);
		g_object_unref((*tagProp)->proxy);
		(*tagProp)->proxy = NULL;
	}
	g_free((*tagProp)->name);
	g_free((*tagProp)->type);
	g_strfreev((*tagProp)->tagType);
	g_free((*tagProp));
	(*tagProp) = NULL;
}

/*****************************************************************************
 * neardal_tag_notify_tag_found: Invoke client callback for 'record found'
 * if present, and 'tag found' (if not already nofied)
 ****************************************************************************/
void neardal_tag_notify_tag_found(TagProp *tagProp)
{
	RcdProp *rcdProp;
	gsize	len;

	NEARDAL_ASSERT(tagProp != NULL);

	if (tagProp->notified == FALSE && neardalMgr.cb.tag_found != NULL) {
		(neardalMgr.cb.tag_found)(tagProp->name,
					   neardalMgr.cb.tag_found_ud);
		tagProp->notified = TRUE;
	}

	len = 0;
	if (neardalMgr.cb.rcd_found != NULL)
		while (len < g_list_length(tagProp->rcdList)) {
			rcdProp = g_list_nth_data(tagProp->rcdList, len++);
			if (rcdProp->notified == FALSE) {
				(neardalMgr.cb.rcd_found)(rcdProp->name,
						neardalMgr.cb.rcd_found_ud);
				rcdProp->notified = TRUE;
			}
		}
}

static void read_completed_cb(GObject *source_object,
				GAsyncResult *res, gpointer user_data)
{
	GError		*gerror		= NULL;
	GVariant	*ret		= NULL;
	NEARDAL_TRACE("Read Completed!");

	ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object),
							res, &gerror);

	if (gerror != NULL) {
		NEARDAL_TRACE_ERR(
		"Unable to read tag's NDEF as a raw bytes stream(%d:%s)\n",
				 gerror->code, gerror->message);
		g_error_free(gerror);
		return;
	}

	if (neardalMgr.cb.read_completed != NULL)
		neardalMgr.cb.read_completed(ret,
				neardalMgr.cb.read_completed_ud);

	g_variant_unref(ret);
}

/*****************************************************************************
 * neardal_tag_get_rawNDEF: Get tag's NDEF as a raw bytes stream
 ****************************************************************************/
errorCode_t neardal_tag_get_rawNDEF(const char *tagName)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	TagProp		*tag		= NULL;

	NEARDAL_ASSERT_RET(tagName != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	neardal_prv_construct(&err);
	if (err != NEARDAL_SUCCESS)
		return err;

	if (!(tag = neardal_mgr_tag_search(tagName)))
		return NEARDAL_ERROR_NO_TAG;

	org_neard_tag_call_get_raw_ndef(tag->proxy, NULL,
					read_completed_cb, NULL);

	return err;
}

static void write_completed_cb(GObject *source_object,
					GAsyncResult *res, gpointer user_data)
{
	errorCode_t	err = NEARDAL_SUCCESS;
	NEARDAL_TRACE("Write Completed!");

	g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object),
					res, &neardalMgr.gerror);
	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"DBUS Error (%d): %s\n",
				 neardalMgr.gerror->code,
				neardalMgr.gerror->message);
		err = NEARDAL_ERROR_DBUS;
	}

	if (neardalMgr.cb.write_completed != NULL)
		neardalMgr.cb.write_completed(err,
				neardalMgr.cb.write_completed_ud);
}

errorCode_t neardal_tag_write(neardal_record *record)
{
	GError		*gerror	= NULL;
	errorCode_t	err;
	TagProp		*tag;
	GVariant	*in;

	NEARDAL_ASSERT_RET(record != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	neardal_prv_construct(&err);
	if (err != NEARDAL_SUCCESS)
		return err;

	if (!(tag = neardal_mgr_tag_search(record->name)))
		return NEARDAL_ERROR_NO_TAG;

	in = neardal_record_to_g_variant(record);

	org_neard_tag_call_write(tag->proxy, in, NULL, write_completed_cb, NULL);

	return err;
}

/*****************************************************************************
 * neardal_tag_prv_add: add new NFC tag, initialize DBus Proxy connection,
 * register tag signal
 ****************************************************************************/
errorCode_t neardal_tag_prv_add(gchar *tagName, void *parent)
{
	errorCode_t	err		= NEARDAL_ERROR_NO_MEMORY;
	TagProp		*tagProp	= NULL;
	AdpProp		*adpProp	= parent;

	NEARDAL_ASSERT_RET((tagName != NULL) && (parent != NULL)
			   , NEARDAL_ERROR_INVALID_PARAMETER);

	NEARDAL_TRACEF("Adding tag:%s\n", tagName);
	tagProp = g_try_malloc0(sizeof(TagProp));
	if (tagProp == NULL)
		goto error;

	tagProp->name	= g_strdup(tagName);
	tagProp->parent	= adpProp;

	adpProp->tagList = g_list_prepend(adpProp->tagList, tagProp);
	err = neardal_tag_prv_init(tagProp);

	NEARDAL_TRACEF("NEARDAL LIB tagList contains %d elements\n",
		      g_list_length(adpProp->tagList));

	return err;

error:
	if (tagProp->name != NULL)
		g_free(tagProp->name);
	if (tagProp != NULL)
		g_free(tagProp);

	return err;
}

/*****************************************************************************
 * neardal_tag_prv_remove: remove one NFC tag, unref DBus Proxy connection,
 * unregister tag signal
 ****************************************************************************/
void neardal_tag_prv_remove(TagProp *tagProp)
{
	RcdProp		*rcdProp	= NULL;
	GList		*node;
	AdpProp		*adpProp;

	NEARDAL_ASSERT(tagProp != NULL);

	NEARDAL_TRACEF("Removing tag:%s\n", tagProp->name);
	/* Remove all tags */
	while (g_list_length(tagProp->rcdList)) {
		node = g_list_first(tagProp->rcdList);
		rcdProp = (RcdProp *) node->data;
		neardal_rcd_remove(rcdProp);
	}
	adpProp = tagProp->parent;
	adpProp->tagList = g_list_remove(adpProp->tagList,
					 (gconstpointer) tagProp);

	neardal_tag_prv_free(&tagProp);
}
