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
 *     This file contains all Public APIs
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "neard_manager_proxy.h"
#include "neard_adapter_proxy.h"

#include "neardal.h"
#include "neardal_prv.h"


#define NEARDAL_SET_STRING_VALUE(gValue, value) do { \
		gValue = g_variant_new_string(value); \
		 } while (0);

#define NEARDAL_SET_BOOL_VALUE(gValue, value)  do { \
		gValue = g_variant_new_boolean((gboolean) value); \
		 } while (0);

#define	ADP_MODE_INITIATOR		"Initiator"
#define	ADP_MODE_TARGET			"Target"
#define	ADP_MODE_DUAL			"Dual"

neardalCtx neardalMgr = {.proxy = NULL};

/*---------------------------------------------------------------------------
 * Context Management
 ---------------------------------------------------------------------------*/
/*****************************************************************************
 * neardal_prv_construct: create NEARDAL object instance, Neard Dbus
 * connection, register Neard's events
 ****************************************************************************/
void neardal_prv_construct(errorCode_t *ec)
{
	errorCode_t	err = NEARDAL_SUCCESS;

	if (neardalMgr.proxy != NULL)
		return;

	NEARDAL_TRACEIN();
	memset(&neardalMgr.conn, 0, sizeof(neardalCtx) - sizeof(neardalCb));
	/* Create DBUS connection */
	g_type_init();
	neardalMgr.conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL,
					   &neardalMgr.gerror);
	if (neardalMgr.conn != NULL) {
		/* We have a DBUS connection, create proxy on Neard Manager */
		err =  neardal_mgr_create();
		if (err != NEARDAL_SUCCESS) {
			NEARDAL_TRACEF(
				"neardal_mgr_create() exit (err %d: %s)\n",
				err, neardal_error_get_text(err));

		}
		/* No Neard daemon, destroying neardal object... */
		if (err == NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY)
			neardal_tools_prv_free_gerror(&neardalMgr.gerror);
	} else {
		NEARDAL_TRACE_ERR("Unable to connect to dbus: %s\n",
				 neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		err = NEARDAL_ERROR_DBUS;
	}

	if (ec != NULL)
		*ec = err;

	NEARDAL_TRACEF("Exit\n");
	return;
}


/*****************************************************************************
 * neardal_destroy: destroy NEARDAL object instance, Disconnect Neard Dbus
 * connection, unregister Neard's events
 ****************************************************************************/
void neardal_destroy(void)
{
	NEARDAL_TRACEIN();
	if (neardalMgr.proxy != NULL) {
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		neardal_mgr_destroy();
	}
}

/*****************************************************************************
 * neardal_set_cb_adapter_added: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_added = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_adapter_added(adapter_cb cb_adp_added,
					 void *user_data)
{
	neardalMgr.cb.adp_added		= cb_adp_added;
	neardalMgr.cb.adp_added_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_cb_adapter_removed: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_removed = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_adapter_removed(adapter_cb cb_adp_removed,
					   void *user_data)
{

	neardalMgr.cb.adp_removed	= cb_adp_removed;
	neardalMgr.cb.adp_removed_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_manager_cb_property_changed: setup a client callback for
 * 'NEARDAL Adapter Property Change'.
 * cb_mgr_adp_property_changed = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_adapter_property_changed(
				adapter_prop_cb cb_adp_property_changed,
					void *user_data)
{
	neardalMgr.cb.adp_prop_changed		= cb_adp_property_changed;
	neardalMgr.cb.adp_prop_changed_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_cb_tag_found: setup a client callback for
 * 'NEARDAL TAG FOUND'.
 * cb_tag_found = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_tag_found(tag_cb cb_tag_found,
					void *user_data)
{
	neardalMgr.cb.tag_found		= cb_tag_found;
	neardalMgr.cb.tag_found_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_cb_tag_lost: setup a client callback for
 * 'NEARDAL TAG LOST'.
 * cb_tag_lost = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_tag_lost(tag_cb cb_tag_lost,
				       void *user_data)
{
	neardalMgr.cb.tag_lost		= cb_tag_lost;
	neardalMgr.cb.tag_lost_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_cb_dev_found: setup a client callback for
 * 'NEARDAL DEVICE FOUND'.
 * cb_dev_found = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_dev_found(dev_cb cb_dev_found,
					void *user_data)
{
	neardalMgr.cb.dev_found		= cb_dev_found;
	neardalMgr.cb.dev_found_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_set_cb_dev_lost: setup a client callback for
 * 'NEARDAL DEVICE LOST'.
 * cb_dev_lost = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_dev_lost(dev_cb cb_dev_lost,
				       void *user_data)
{
	neardalMgr.cb.dev_lost		= cb_dev_lost;
	neardalMgr.cb.dev_lost_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}


/*****************************************************************************
 * neardal_set_cb_record_found: setup a client callback for
 * 'NEARDAL tag record found'.
 * cb_rcd_found = NULL to remove actual callback.
 ****************************************************************************/
errorCode_t neardal_set_cb_record_found(record_cb cb_rcd_found,
					void *user_data)
{
	neardalMgr.cb.rcd_found		= cb_rcd_found;
	neardalMgr.cb.rcd_found_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

errorCode_t neardal_set_cb_power_completed(power_cb cb_power_completed,
					void *user_data)
{
	neardalMgr.cb.power_completed		= cb_power_completed;
	neardalMgr.cb.power_completed_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

errorCode_t neardal_set_cb_read_completed(read_cb cb_read_completed,
					void *user_data)
{
	neardalMgr.cb.read_completed		= cb_read_completed;
	neardalMgr.cb.read_completed_ud		= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

errorCode_t neardal_set_cb_write_completed(write_cb cb_write_completed,
					void *user_data)
{
	neardalMgr.cb.write_completed		= cb_write_completed;
	neardalMgr.cb.write_completed_ud	= user_data;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(NULL);

	return NEARDAL_SUCCESS;
}

/*****************************************************************************
 * neardal_free_array: free adapters array, tags array or records array
 ****************************************************************************/
errorCode_t neardal_free_array(char ***array)
{
	errorCode_t	err = NEARDAL_SUCCESS;
	char		**adps;

	if (array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	if (*array == NULL)
		return NEARDAL_ERROR_GENERAL_ERROR;

	adps = *array;
	while ((*adps) != NULL) {
		g_free(*adps);
		adps++;
	}
	g_free(*array);
	*array = NULL;

	return err;
}

/*****************************************************************************
 * neardal_error_get_text: return string error form error code
 ****************************************************************************/
char *neardal_error_get_text(errorCode_t ec)
{
	switch (ec) {
	case NEARDAL_SUCCESS:
		return "Success";

	case NEARDAL_ERROR_GENERAL_ERROR:
		return "General error";

	case NEARDAL_ERROR_INVALID_PARAMETER:
		return "Invalid parameter";

	case NEARDAL_ERROR_NO_MEMORY:
		return "Memory allocation error";

	case NEARDAL_ERROR_DBUS:
		return "DBUS general error";

	case NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY:
		return "Can not create a DBUS proxy";

	case NEARDAL_ERROR_DBUS_CANNOT_INVOKE_METHOD:
		return "Can not invoke a DBUS method";

	case NEARDAL_ERROR_NO_ADAPTER:
		return "No NFC adapter found...";

	case NEARDAL_ERROR_NO_TAG:
		return "No NFC tag found...";

	case NEARDAL_ERROR_NO_DEV:
		return "No NFC device found...";

	case NEARDAL_ERROR_NO_RECORD:
		return "No tag record found...";

	case NEARDAL_ERROR_INVALID_RECORD:
		return "Tag record has an invalid format...";

	case NEARDAL_ERROR_POLLING_ALREADY_ACTIVE:
		return "Polling already active";

	case NEARDAL_ERROR_DBUS_INVOKE_METHOD_ERROR:
		return "Error while invoking method";
	}

	return "UNKNOWN ERROR !!!";
}


/*---------------------------------------------------------------------------
 * NFC Adapter Management
 ---------------------------------------------------------------------------*/
/*****************************************************************************
 * neardal_get_adapters: get an array of NFC adapters (adpName) present
 ****************************************************************************/
errorCode_t neardal_get_adapters(char ***array, int *len)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	int		adpNb		= 0;
	int		ct		= 0;	/* counter */
	char		**adps		= NULL;
	AdpProp		*adapter	= NULL;
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	adpNb = g_list_length(neardalMgr.prop.adpList);
	if (adpNb > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (adpNb + 1) * sizeof(char *);
		adps = g_try_malloc0(size);
		if (adps != NULL) {
			GList	*list;
			while (ct < adpNb) {
				list = neardalMgr.prop.adpList;
				adapter = g_list_nth_data(list, ct);
				if (adapter != NULL)
					adps[ct++] = g_strdup(adapter->name);
			}
			err = NEARDAL_SUCCESS;
		}
	} else
		err = NEARDAL_ERROR_NO_ADAPTER;

	if (len != NULL)
		*len = adpNb;
	*array	= adps;

	return err;
}

/*****************************************************************************
 * neardal_free_adapter: Release memory allocated for properties of an adapter
 ****************************************************************************/
void neardal_free_adapter(neardal_adapter *adapter)
{
	int ct		= 0;	/* counter */

	if (adapter == NULL) {
		NEARDAL_TRACE_ERR("Adapter provided is NULL!\n");
		return;
	}

	/* Freeing adapter name */
	g_free(adapter->name);

	/* Freeing adapter mode */
	g_free(adapter->mode);

	// Freeing protocols list
	ct = 0;
	while (ct < adapter->nbProtocols)
		g_free(adapter->protocols[ct++]);
	g_free(adapter->protocols);

	/* Freeing tags list */
	ct = 0;
	while (ct < adapter->nbTags)
		g_free(adapter->tags[ct++]);
	g_free(adapter->tags);

	/* Freeing adapter struct */
	g_free(adapter);
}

/*****************************************************************************
 * neardal_get_adapter_properties: Get properties of a specific NEARDAL adapter
 ****************************************************************************/
errorCode_t neardal_get_adapter_properties(const char *adpName,
					   neardal_adapter **adapter)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TagProp		*tag		= NULL;
	neardal_adapter	*adpClient	= NULL;
	int		ct		= 0;	/* counter */
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || adpName == NULL || adapter == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) adpName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	adpClient = g_try_malloc0(sizeof(neardal_adapter));
	if (adpClient == NULL) {
		err = NEARDAL_ERROR_NO_MEMORY;
		goto exit;
	}
	*adapter = adpClient;

	adpClient->name		= g_strdup(adpProp->name);
	adpClient->mode		= g_strdup(adpProp->mode);
	adpClient->polling	= (short) adpProp->polling;
	adpClient->powered	= (short) adpProp->powered;

	adpClient->nbProtocols	= adpProp->lenProtocols;
	adpClient->protocols	= NULL;


	if (adpClient->nbProtocols > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (adpClient->nbProtocols + 1) * sizeof(char *);
		adpClient->protocols = g_try_malloc0(size);
		if (adpClient->protocols != NULL) {
			ct = 0;
			while (ct < adpClient->nbProtocols) {
				gchar *tmp = g_strdup(adpProp->protocols[ct]);
				adpClient->protocols[ct++] = (char *) tmp;
			}
			err = NEARDAL_SUCCESS;
		}
	}

	adpClient->nbTags	= (int) adpProp->tagNb;
	adpClient->tags	= NULL;
	if (adpClient->nbTags <= 0)
		goto exit;

	err = NEARDAL_ERROR_NO_MEMORY;
	size = (adpClient->nbTags + 1) * sizeof(char *);
	adpClient->tags = g_try_malloc0(size);
	if (adpClient->tags == NULL)
		goto exit;

	ct = 0;
	while (ct < adpClient->nbTags) {
		tag = g_list_nth_data(adpProp->tagList, ct);
		if (tag != NULL)
			adpClient->tags[ct++] = g_strdup(tag->name);
	}
	err = NEARDAL_SUCCESS;

exit:
	if (err != NEARDAL_SUCCESS) {
		neardal_free_adapter(adpClient);
		if (adapter != NULL)
			*adapter = NULL;
	}

	return err;
}

static void power_completed_cb(GObject *source_object,
					GAsyncResult *res, gpointer user_data)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;

	NEARDAL_TRACE("Power Completed!");
	err = neardal_mgr_prv_get_adapter((gchar *) user_data, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	if (adpProp->proxy == NULL) {
		NEARDAL_TRACE("adapter proxy is null");
		err = NEARDAL_ERROR_NO_ADAPTER;
		goto exit;
	}

	org_neard_adp__call_set_property_finish(adpProp->proxy,
						res, &neardalMgr.gerror);
	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"DBUS Error (%d): %s\n",
				 neardalMgr.gerror->code,
				neardalMgr.gerror->message);
		err = NEARDAL_ERROR_DBUS;
	}

exit:
	if (neardalMgr.cb.power_completed != NULL)
		neardalMgr.cb.power_completed(err,
					neardalMgr.cb.power_completed_ud);

	g_free(user_data);
}

/*****************************************************************************
 * neardal_set_adapter_property: Set a property on a specific NEARDAL adapter
 ****************************************************************************/
errorCode_t neardal_set_adapter_property(const char *adpName,
					   int adpPropId, void *value)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	const gchar	*propKey	= NULL;
	GVariant	*propValue	= NULL;
	GVariant	*variantTmp	= NULL;
	gchar		*user_data	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || adpName == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) adpName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	switch (adpPropId) {
	case NEARD_ADP_PROP_POWERED:
		propKey = "Powered";
		NEARDAL_SET_BOOL_VALUE(variantTmp, value);
		break;
	default:
		break;
	}

	propValue = g_variant_new_variant(variantTmp);
	NEARDAL_TRACE_LOG("Sending:\n%s=%s\n", propKey,
			  g_variant_print(propValue, TRUE));

	user_data = g_strdup(adpName);
	org_neard_adp__call_set_property(adpProp->proxy, propKey,
					propValue, NULL, power_completed_cb,
					user_data);


exit:
	neardal_tools_prv_free_gerror(&neardalMgr.gerror);
	return err;
}

/*****************************************************************************
 * neardal_start_poll: Request Neard to start polling
 ****************************************************************************/
errorCode_t neardal_start_poll_loop(char *adpName, int mode)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);
	if (err != NEARDAL_SUCCESS)
		return err;

	err = neardal_mgr_prv_get_adapter(adpName, &adpProp);

	if (adpProp == NULL)
		goto exit;

	if (adpProp->proxy == NULL)
		goto exit;

	if (adpProp->polling) {
		err = NEARDAL_ERROR_POLLING_ALREADY_ACTIVE;
		goto exit;
	}

	if (mode == NEARD_ADP_MODE_INITIATOR)
		org_neard_adp__call_start_poll_loop_sync(adpProp->proxy,
							ADP_MODE_INITIATOR,
							NULL,
							&neardalMgr.gerror);
	else if (mode == NEARD_ADP_MODE_TARGET)
		org_neard_adp__call_start_poll_loop_sync(adpProp->proxy,
							ADP_MODE_TARGET, NULL,
							&neardalMgr.gerror);
	else if (mode == NEARD_ADP_MODE_DUAL)
		org_neard_adp__call_start_poll_loop_sync(adpProp->proxy,
							ADP_MODE_DUAL, NULL,
							&neardalMgr.gerror);
	else
		org_neard_adp__call_start_poll_loop_sync(adpProp->proxy,
						ADP_MODE_INITIATOR, NULL,
							&neardalMgr.gerror);

	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"Error with neard dbus method (err:%d:'%s')\n"
				, neardalMgr.gerror->code
				, neardalMgr.gerror->message);
		err = NEARDAL_ERROR_DBUS_INVOKE_METHOD_ERROR;
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
	}

exit:
	return err;
}

/*****************************************************************************
 * neardal_stop_poll: Request Neard to stop polling
 ****************************************************************************/
errorCode_t neardal_stop_poll(char *adpName)
{
	errorCode_t	err = NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err == NEARDAL_SUCCESS)
		err = neardal_mgr_prv_get_adapter(adpName, &adpProp);

	if (adpProp == NULL)
		goto exit;

	if (adpProp->proxy == NULL)
		goto exit;

	if (adpProp->polling) {
		org_neard_adp__call_stop_poll_loop_sync(adpProp->proxy, NULL,
						   &neardalMgr.gerror);

		err = NEARDAL_SUCCESS;
		if (neardalMgr.gerror != NULL) {
			NEARDAL_TRACE_ERR(
				"Error with neard dbus method (err:%d:'%s')\n"
					, neardalMgr.gerror->code
					, neardalMgr.gerror->message);
			err = NEARDAL_ERROR_DBUS_INVOKE_METHOD_ERROR;
			neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		}
	}

exit:
	return err;
}


/*---------------------------------------------------------------------------
 * NFC Tag Management
 ---------------------------------------------------------------------------*/
/*****************************************************************************
 * neardal_get_tags: get an array of NFC tags present
 ****************************************************************************/
errorCode_t neardal_get_tags(char *adpName, char ***array, int *len)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	int		tagNb		= 0;
	int		ct		= 0;	/* counter */
	char		**tags		= NULL;
	TagProp		*tag		= NULL;


	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);
	else
		err = NEARDAL_ERROR_NO_TAG;

	if (adpName == NULL || array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	err = neardal_mgr_prv_get_adapter(adpName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		return err;

	tagNb = g_list_length(adpProp->tagList);
	if (tagNb <= 0)
		return NEARDAL_ERROR_NO_TAG;

	err = NEARDAL_ERROR_NO_MEMORY;
	tags = g_try_malloc0((tagNb + 1) * sizeof(char *));

	if (tags == NULL)
		return NEARDAL_ERROR_NO_MEMORY;

	while (ct < tagNb) {
		tag = g_list_nth_data(adpProp->tagList, ct);
		if (tag != NULL)
			tags[ct++] = g_strdup(tag->name);
	}
	err = NEARDAL_SUCCESS;

	if (len != NULL)
		*len = tagNb;
	*array	= tags;

	return err;
}

/*****************************************************************************
 * neardal_free_tag: Release memory allocated for properties of a tag
 ****************************************************************************/
void neardal_free_tag(neardal_tag *tag)
{
	int	ct	= 0;	/* counter */

	if (tag == NULL) {
		NEARDAL_TRACE_ERR("Tag provided is NULL!\n");
		return;
	}

	/* Freeing tag name/type */
	g_free((gpointer) tag->name);
	g_free((gpointer) tag->type);

	/* Freeing records list */
	ct = 0;
	while (ct < tag->nbRecords)
		g_free(tag->records[ct++]);
	g_free(tag->records);

	/* Freeing tag type list */
	ct = 0;
	while (ct < tag->nbTagTypes)
		g_free(tag->tagType[ct++]);
	g_free(tag->tagType);

	/* Freeing adapter struct */
	g_free(tag);
}

/*****************************************************************************
 * neardal_get_tag_properties: Get properties of a specific NEARDAL
 * tag
 ****************************************************************************/
errorCode_t neardal_get_tag_properties(const char *tagName,
					  neardal_tag **tag)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TagProp		*tagProp	= NULL;
	neardal_tag	*tagClient	= NULL;
	int		ct		= 0;	/* counter */
	RcdProp		*record		= NULL;
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || tagName == NULL || tag == NULL)
		goto exit;

	tagClient = g_try_malloc0(sizeof(neardal_tag));
	if (tagClient == NULL) {
		err = NEARDAL_ERROR_NO_MEMORY;
		goto exit;
	}
	*tag = tagClient;

	tagClient->records	= NULL;
	tagClient->tagType	= NULL;
	err = neardal_mgr_prv_get_adapter((gchar *) tagName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_adp_prv_get_tag(adpProp, (gchar *) tagName, &tagProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	tagClient->name		= g_strdup(tagProp->name);
	tagClient->type		= g_strdup(tagProp->type);
	tagClient->readOnly	= (short) tagProp->readOnly;
	tagClient->nbRecords	= (int) tagProp->rcdLen;
	if (tagClient->nbRecords > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (tagClient->nbRecords + 1) * sizeof(char *);
		tagClient->records = g_try_malloc0(size);
		if (tagClient->records == NULL)
			goto exit;

		ct = 0;
		while (ct < tagClient->nbRecords) {
			record = g_list_nth_data(tagProp->rcdList, ct);
			if (record != NULL)
				tagClient->records[ct] = g_strdup(record->name);
			ct++;
		}
		err = NEARDAL_SUCCESS;
	}

	tagClient->nbTagTypes = 0;
	tagClient->tagType = NULL;
	/* Count TagTypes */
	tagClient->nbTagTypes = (int) tagProp->tagTypeLen;

	if (tagClient->nbTagTypes <= 0)
		goto exit;

	err = NEARDAL_ERROR_NO_MEMORY;
	size = (tagClient->nbTagTypes + 1) * sizeof(char *);
	tagClient->tagType = g_try_malloc0(size);
	if (tagClient->tagType == NULL)
		goto exit;

	ct = 0;
	while (ct < tagClient->nbTagTypes) {
		tagClient->tagType[ct] = g_strdup(tagProp->tagType[ct]);
		ct++;
	}
	err = NEARDAL_SUCCESS;

exit:
	if (err != NEARDAL_SUCCESS) {
		neardal_free_tag(tagClient);
		if (tag != NULL)
			*tag = NULL;
	}

	return err;
}

/*****************************************************************************
 * neardal_tag_get_rawNDEF: Get tag's NDEF as a raw bytes stream
 ****************************************************************************/
errorCode_t neardal_tag_get_rawNDEF(const char *tagName)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TagProp		*tagProp	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || tagName == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) tagName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_adp_prv_get_tag(adpProp, (gchar *) tagName, &tagProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_tag_prv_get_raw_NDEF(tagProp);

exit:
	return err;
}

/*****************************************************************************
 * neardal_tag_write: Write NDEF record to an NFC tag
 ****************************************************************************/
errorCode_t neardal_tag_write(neardal_record *record)
{
	errorCode_t	err	= NEARDAL_SUCCESS;
	AdpProp		*adpProp;
	TagProp		*tagProp;
	RcdProp		rcd;
	GVariantBuilder	*builder;
	guint32		count;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || record == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) record->name, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;
	err = neardal_adp_prv_get_tag(adpProp, (gchar *) record->name,
				      &tagProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	rcd.name		= (gchar *) record->name;
	rcd.action		= (gchar *) record->action;
	rcd.encoding		= (gchar *) record->encoding;
	rcd.language		= (gchar *) record->language;
	rcd.type		= (gchar *) record->type;
	rcd.representation	= (gchar *) record->representation;
	rcd.uri			= (gchar *) record->uri;
	rcd.uriObjSize		= record->uriObjSize;
	rcd.mime		= (gchar *) record->mime;

	if (record->rawNDEF != NULL && record->rawNDEFSize > 0) {

		builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));

		for (count = 0; count < record->rawNDEFSize; count++)
			g_variant_builder_add(builder, "y",
						record->rawNDEF[count]);

		rcd.rawNDEF = g_variant_new("ay", builder);
		g_variant_builder_unref(builder);
	}

	err = neardal_tag_prv_write(tagProp, &rcd);

exit:
	return err;
}

/*---------------------------------------------------------------------------
 * NFC Dev Management
 ---------------------------------------------------------------------------*/
/*****************************************************************************
 * neardal_get_devices: get an array of NFC devs present
 ****************************************************************************/
errorCode_t neardal_get_devices(char *adpName, char ***array, int *len)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	int		devNb		= 0;
	int		ct		= 0;	/* counter */
	char		**devs		= NULL;
	DevProp		*dev		= NULL;


	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);
	else
		err = NEARDAL_ERROR_NO_DEV;

	if (adpName == NULL || array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	err = neardal_mgr_prv_get_adapter(adpName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		return err;

	devNb = g_list_length(adpProp->devList);
	if (devNb <= 0)
		return NEARDAL_ERROR_NO_DEV;

	err = NEARDAL_ERROR_NO_MEMORY;
	devs = g_try_malloc0((devNb + 1) * sizeof(char *));

	if (devs == NULL)
		return NEARDAL_ERROR_NO_MEMORY;

	while (ct < devNb) {
		dev = g_list_nth_data(adpProp->devList, ct);
		if (dev != NULL)
			devs[ct++] = g_strdup(dev->name);
	}
	err = NEARDAL_SUCCESS;

	if (len != NULL)
		*len = devNb;
	*array	= devs;

	return err;
}

/*****************************************************************************
 * neardal_free_device: Release memory allocated for properties of a dev
 ****************************************************************************/
void neardal_free_device(neardal_dev *dev)
{
	int	ct	= 0;	/* counter */

	if (dev == NULL) {
		NEARDAL_TRACE_ERR("Dev provided is NULL!\n");
		return;
	}

	/* Freeing dev name/type */
	g_free((gpointer) dev->name);

	/* Freeing records list */
	ct = 0;
	while (ct < dev->nbRecords)
		g_free(dev->records[ct++]);
	g_free(dev->records);

	/* Freeing adapter struct */
	g_free(dev);
}

/*****************************************************************************
 * neardal_get_dev_properties: Get properties of a specific NEARDAL
 * dev
 ****************************************************************************/
errorCode_t neardal_get_dev_properties(const char *devName,
					  neardal_dev **dev)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	DevProp		*devProp	= NULL;
	neardal_dev	*devClient	= NULL;
	int		ct		= 0;	/* counter */
	RcdProp		*record		= NULL;
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || devName == NULL || dev == NULL)
		goto exit;

	devClient = g_try_malloc0(sizeof(neardal_dev));
	if (devClient == NULL) {
		err = NEARDAL_ERROR_NO_MEMORY;
		goto exit;
	}
	*dev = devClient;

	devClient->records	= NULL;
	err = neardal_mgr_prv_get_adapter((gchar *) devName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_adp_prv_get_dev(adpProp, (gchar *) devName, &devProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	devClient->name		= g_strdup(devProp->name);
	devClient->nbRecords	= (int) devProp->rcdLen;
	if (devClient->nbRecords > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (devClient->nbRecords + 1) * sizeof(char *);
		devClient->records = g_try_malloc0(size);
		if (devClient->records == NULL)
			goto exit;

		ct = 0;
		while (ct < devClient->nbRecords) {
			record = g_list_nth_data(devProp->rcdList, ct);
			if (record != NULL)
				devClient->records[ct++] = g_strdup(
								record->name);
		}
		err = NEARDAL_SUCCESS;
	}

	err = NEARDAL_SUCCESS;

exit:
	if (err != NEARDAL_SUCCESS) {
		neardal_free_device(devClient);
		if (dev != NULL)
			*dev = NULL;
	}

	return err;
}

/*****************************************************************************
 * neardal_dev_push: Push NDEF record to an NFC dev
 ****************************************************************************/
errorCode_t neardal_dev_push(neardal_record *record)
{
	errorCode_t	err	= NEARDAL_SUCCESS;
	AdpProp		*adpProp;
	DevProp		*devProp;
	RcdProp		rcd;


	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || record == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) record->name, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;
	err = neardal_adp_prv_get_dev(adpProp, (gchar *) record->name,
				      &devProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	rcd.name		= (gchar *) record->name;
	rcd.action		= (gchar *) record->action;
	rcd.encoding		= (gchar *) record->encoding;
	rcd.language		= (gchar *) record->language;
	rcd.type		= (gchar *) record->type;
	rcd.representation	= (gchar *) record->representation;
	rcd.uri			= (gchar *) record->uri;
	rcd.uriObjSize		= record->uriObjSize;
	rcd.mime		= (gchar *) record->mime;

	neardal_dev_prv_push(devProp, &rcd);
exit:
	return err;
}



/*---------------------------------------------------------------------------
 * NFC Record Management
 ---------------------------------------------------------------------------*/
/******************************************************************************
 * neardal_get_records: get an array of tag records
 *****************************************************************************/
errorCode_t neardal_get_records(char *tagName, char ***array, int *len)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TagProp		*tagProp	= NULL;
	int		rcdLen		= 0;
	int		ct		= 0;	/* counter */
	char		**rcds		= NULL;
	RcdProp		*rcd		= NULL;


	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || tagName == NULL || array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	err = neardal_mgr_prv_get_adapter(tagName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_adp_prv_get_tag(adpProp, tagName, &tagProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err		= NEARDAL_ERROR_NO_RECORD;
	rcdLen = g_list_length(tagProp->rcdList);
	if (rcdLen <= 0)
		goto exit;

	err = NEARDAL_ERROR_NO_MEMORY;
	rcds = g_try_malloc0((rcdLen + 1) * sizeof(char *));
	if (rcds == NULL)
		goto exit;

	while (ct < rcdLen) {
		rcd = g_list_nth_data(tagProp->rcdList, ct);
		if (rcd != NULL)
			rcds[ct++] = g_strdup(rcd->name);
	}
	err = NEARDAL_SUCCESS;

exit:
	if (len != NULL)
		*len = rcdLen;
	*array	= rcds;

	return err;
}

/*****************************************************************************
 * neardal_free_record: Release memory allocated for properties of a record
 ****************************************************************************/
void neardal_free_record(neardal_record *record)
{
	if (record == NULL) {
		NEARDAL_TRACE_ERR("Record provided is NULL!\n");
		return;
	}

	/* Freeing record properties */
	g_free((gpointer) record->name);
	g_free((gpointer) record->encoding);
	g_free((gpointer) record->language);
	g_free((gpointer) record->action);
	g_free((gpointer) record->type);
	g_free((gpointer) record->representation);
	g_free((gpointer) record->uri);
	g_free((gpointer) record->mime);
	g_free((gpointer) record->rawNDEF);

	/* Freeing record struct */
	g_free(record);
}

/*****************************************************************************
 * neardal_get_record_properties: Get values of a specific tag record
 ****************************************************************************/
errorCode_t neardal_get_record_properties(const char *recordName,
					  neardal_record **record)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TagProp		*tagProp	= NULL;
	RcdProp		*rcdProp	= NULL;
	neardal_record *rcdClient	= NULL;

	if (recordName == NULL || record == NULL)
		goto exit;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) recordName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_adp_prv_get_tag(adpProp, (gchar *) recordName,
					 &tagProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_tag_prv_get_record(tagProp, (gchar *) recordName,
					 &rcdProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	rcdClient = g_try_malloc0(sizeof(neardal_record));
	if (rcdClient == NULL) {
		err = NEARDAL_ERROR_NO_MEMORY;
		goto exit;
	}
	*record = rcdClient;

	rcdClient->name			= g_strdup(rcdProp->name);
	rcdClient->encoding		= g_strdup(rcdProp->encoding);
	rcdClient->language		= g_strdup(rcdProp->language);
	rcdClient->action		= g_strdup(rcdProp->action);

	rcdClient->type			= g_strdup(rcdProp->type);
	rcdClient->representation	= g_strdup(rcdProp->representation);
	rcdClient->uri			= g_strdup(rcdProp->uri);
	rcdClient->uriObjSize		= (unsigned int) rcdProp->uriObjSize;
	rcdClient->mime			= g_strdup(rcdProp->mime);

exit:
	if (err != NEARDAL_SUCCESS) {
		neardal_free_record(rcdClient);
		if (record != NULL)
			*record = NULL;
	}

	return err;
}


