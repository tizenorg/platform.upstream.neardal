/*
 *     NEARDAL (Neard Abstraction Library)
 *
 *     Copyright 2012 Intel Corporation. All rights reserved.
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

#include "neard_record_proxy.h"

#include "neardal.h"
#include "neardal_prv.h"


/*****************************************************************************
 * neardal_rcd_prv_read_properties: Get Neard Record Properties
 ****************************************************************************/
static errorCode_t neardal_rcd_prv_read_properties(RcdProp *rcd)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	GError		*gerror		= NULL;
	GVariant	*tmp		= NULL;
	GVariant	*tmpOut		= NULL;

	NEARDAL_TRACEIN();
	NEARDAL_ASSERT_RET(rcd != NULL, NEARDAL_ERROR_INVALID_PARAMETER);
	NEARDAL_ASSERT_RET(rcd->proxy != NULL
			   , NEARDAL_ERROR_INVALID_PARAMETER);

	org_neard_rcd__call_get_properties_sync(rcd->proxy, &tmp, NULL,
						&gerror);
	if (gerror != NULL) {
		err = NEARDAL_ERROR_DBUS;
		NEARDAL_TRACE_ERR(
			"Unable to read record's properties (%d:%s)\n",
				   gerror->code, gerror->message);
		g_error_free(gerror);
		return err;
	}
	NEARDAL_TRACEF("Reading:\n%s\n", g_variant_print(tmp, TRUE));

	tmpOut = g_variant_lookup_value(tmp, "Type", G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->type = g_variant_dup_string(tmpOut, NULL);
	else
		goto error;

	tmpOut = g_variant_lookup_value(tmp, "Representation",
					G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->representation = g_variant_dup_string(tmpOut,
								NULL);

	tmpOut = g_variant_lookup_value(tmp, "Encoding", G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->encoding = g_variant_dup_string(tmpOut, NULL);

	tmpOut = g_variant_lookup_value(tmp, "Language", G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->language = g_variant_dup_string(tmpOut, NULL);

	tmpOut = g_variant_lookup_value(tmp, "MIME",
					G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->mime = g_variant_dup_string(tmpOut, NULL);

	tmpOut = g_variant_lookup_value(tmp, "URI",
					G_VARIANT_TYPE_STRING);
	if (tmpOut != NULL)
		rcd->uri = g_variant_dup_string(tmpOut, NULL);

	return err;
error:
	/* due to error, record content will be destroyed later */
	return NEARDAL_ERROR_INVALID_RECORD;
}

/*****************************************************************************
 * neardal_rcd_init: Get Neard Manager Properties = NFC Records list.
 * Create a DBus proxy for the first one NFC record if present
 * Register Neard Manager signals ('PropertyChanged')
 ****************************************************************************/
static errorCode_t neardal_rcd_prv_init(RcdProp *rcd)
{
	NEARDAL_TRACEIN();
	NEARDAL_ASSERT_RET(rcd != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	if (rcd->proxy != NULL)
		g_object_unref(rcd->proxy);
	rcd->proxy = NULL;

	rcd->proxy = org_neard_rcd__proxy_new_sync(neardalMgr.conn,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
							NEARD_DBUS_SERVICE,
							rcd->name,
							NULL, /* GCancellable */
							&neardalMgr.gerror);
	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"Unable to create Neard Record Proxy (%d:%s)\n",
				 neardalMgr.gerror->code,
				neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		return NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY;
	}

	return neardal_rcd_prv_read_properties(rcd);
}

/*****************************************************************************
 * neardal_rcd_prv_free: unref DBus proxy, disconnect Neard Record signals
 ****************************************************************************/
void neardal_rcd_prv_free(RcdProp **rcd)
{
	NEARDAL_TRACEIN();
	if ((*rcd)->proxy != NULL)
		g_object_unref((*rcd)->proxy);
	(*rcd)->proxy = NULL;
	g_free((*rcd)->name);
	g_free((*rcd)->language);
	g_free((*rcd)->encoding);
	g_free((*rcd)->mime);
	g_free((*rcd)->representation);
	g_free((*rcd)->type);
	g_free((*rcd)->uri);
	g_free((*rcd));
	(*rcd) = NULL;
}

/*****************************************************************************
 * neardal_rcd_prv_format: Insert key/value in a GHashTable
 ****************************************************************************/
errorCode_t neardal_rcd_prv_format(GVariantBuilder *builder, RcdProp *rcd)
{
	errorCode_t	err		= NEARDAL_SUCCESS;


	NEARDAL_TRACEIN();
	NEARDAL_ASSERT_RET(rcd != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	/* Type */
	if (rcd->type != NULL)
		neardal_tools_prv_add_dict_entry(builder, "Type", rcd->type,
					    (int) G_TYPE_STRING);

	/* Encoding */
	if (rcd->encoding != NULL)
		neardal_tools_prv_add_dict_entry(builder, "Encoding"
						 , rcd->encoding
						 , (int) G_TYPE_STRING);

	/* Language */
	if (rcd->language != NULL)
		neardal_tools_prv_add_dict_entry(builder, "Language"
						 , rcd->language
						 , (int) G_TYPE_STRING);

	/* Representation */
	if (rcd->representation != NULL)
		neardal_tools_prv_add_dict_entry(builder, "Representation",
					     rcd->representation,
					    (int) G_TYPE_STRING);

	/* URI */
	if (rcd->uri != NULL) {
		neardal_tools_prv_add_dict_entry(builder, "URI", rcd->uri,
					    (int) G_TYPE_STRING);
		neardal_tools_prv_add_dict_entry(builder, "Size",
					    (void *) rcd->uriObjSize,
					    (int) G_TYPE_UINT);

	}
	/* MIME */
	if (rcd->mime != NULL)
		neardal_tools_prv_add_dict_entry(builder, "MIME", rcd->mime,
						(int) G_TYPE_STRING);

	/* Action */
	if (rcd->action != NULL)
		neardal_tools_prv_add_dict_entry(builder, "Action", rcd->action,
					    (int) G_TYPE_STRING);

	/* RawNDEF*/
	if (rcd->rawNDEF != NULL)
		neardal_tools_prv_add_dict_entry(builder, "NDEF", rcd->rawNDEF,
					    (int) G_TYPE_VARIANT);

	return err;
}


/******************************************************************************
 * neardal_rcd_add: add new NFC record, initialize DBus Proxy connection,
 * register record signal
 *****************************************************************************/
errorCode_t neardal_rcd_add(char *rcdName, void *parent)
{
	errorCode_t	err		= NEARDAL_ERROR_NO_MEMORY;
	RcdProp		*rcd	= NULL;
	TagProp		*tagProp = parent;

	NEARDAL_ASSERT_RET((rcdName != NULL) && (parent != NULL)
			  , NEARDAL_ERROR_INVALID_PARAMETER);

	NEARDAL_TRACEF("Adding record:%s\n", rcdName);
	rcd = g_try_malloc0(sizeof(RcdProp));
	if (rcd == NULL)
		goto exit;

	rcd->name = g_strdup(rcdName);
	if (rcd->name == NULL)
		goto exit;

	rcd->parent = tagProp;

	tagProp->rcdList = g_list_prepend(tagProp->rcdList, (gpointer) rcd);
	err = neardal_rcd_prv_init(rcd);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	NEARDAL_TRACEF("NEARDAL LIB recordList contains %d elements\n",
		      g_list_length(tagProp->rcdList));

	err = NEARDAL_SUCCESS;

exit:
	if (err != NEARDAL_SUCCESS) {
		tagProp->rcdList = g_list_remove(tagProp->rcdList,
						 (gpointer) rcd);
		neardal_rcd_prv_free(&rcd);
	}

	return err;
}

/*****************************************************************************
 * neardal_rcd_remove: remove NFC record, unref DBus Proxy connection,
 * unregister record signal
 ****************************************************************************/
void neardal_rcd_remove(RcdProp *rcd)
{
	TagProp		*tagProp;

	NEARDAL_TRACEIN();
	NEARDAL_ASSERT(rcd != NULL);

	tagProp = rcd->parent;
	NEARDAL_TRACEF("Removing record:%s\n", rcd->name);
	tagProp->rcdList = g_list_remove(tagProp->rcdList,
					 (gconstpointer) rcd);
	/* Remove all records */
	neardal_rcd_prv_free(&rcd);
}
