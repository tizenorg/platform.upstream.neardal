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

#include "neard_manager_proxy.h"
#include "neard_adapter_proxy.h"

#include "neardal.h"
#include "neardal_prv.h"

/*****************************************************************************
 * neardal_mgr_prv_cb_property_changed: Callback called when a NFC Manager
 * Property is changed
 ****************************************************************************/
static void neardal_mgr_prv_cb_property_changed(orgNeardMgr *proxy,
						const gchar *arg_unnamed_arg0,
						GVariant *arg_unnamed_arg1,
						void        *user_data)
{
	NEARDAL_TRACEIN();

	(void) proxy; /* remove warning */
	(void) arg_unnamed_arg1; /* remove warning */
	(void) user_data; /* remove warning */
	NEARDAL_ASSERT(arg_unnamed_arg0 != NULL);

	NEARDAL_TRACEF("arg_unnamed_arg0='%s'\n", arg_unnamed_arg0);
	/* Adapters List ignored... */
}

/*****************************************************************************
 * neardal_mgr_prv_cb_adapter_added: Callback called when a NFC adapter is
 * added
 ****************************************************************************/
static void neardal_mgr_prv_cb_adapter_added(orgNeardMgr *proxy,
					     const gchar *arg_unnamed_arg0,
					     void        *user_data)
{
	errorCode_t	err = NEARDAL_SUCCESS;

	NEARDAL_TRACEIN();
	(void) proxy; /* remove warning */
	(void) user_data; /* remove warning */

	NEARDAL_ASSERT(arg_unnamed_arg0 != NULL);
	
	err = neardal_adp_add((char *) arg_unnamed_arg0);
	if (err != NEARDAL_SUCCESS)
		return;

	NEARDAL_TRACEF("NEARDAL LIB adapterList contains %d elements\n",
		      g_list_length(neardalMgr.prop.adpList));
}

/*****************************************************************************
 * neardal_mgr_prv_cb_adapter_removed: Callback called when a NFC adapter
 * is removed
 ****************************************************************************/
static void neardal_mgr_prv_cb_adapter_removed(orgNeardMgr *proxy,
					       const gchar *arg_unnamed_arg0,
					       void *user_data)
{
	GList	*node	= NULL;

	NEARDAL_TRACEIN();
	(void) proxy; /* remove warning */
	(void) user_data; /* remove warning */

	NEARDAL_ASSERT(arg_unnamed_arg0 != NULL);

	node = g_list_first(neardalMgr.prop.adpList);
	if (node == NULL) {
		NEARDAL_TRACE_ERR("NFC adapter not found! (%s)\n",
				  arg_unnamed_arg0);
		return;
	}

	/* Invoke client cb 'adapter removed' */
	if (neardalMgr.cb.adp_removed != NULL)
		(neardalMgr.cb.adp_removed)((char *) arg_unnamed_arg0,
					 neardalMgr.cb.adp_removed_ud);

	neardal_adp_remove(((AdpProp *)node->data));

	NEARDAL_TRACEF("NEARDAL LIB adapterList contains %d elements\n",
		      g_list_length(neardalMgr.prop.adpList));
}

/*****************************************************************************
 * neardal_mgr_prv_get_all_adapters: Check if neard has an adapter
 ****************************************************************************/
static errorCode_t neardal_mgr_prv_get_all_adapters(gchar ***adpArray,
						    gsize *len)
{
	errorCode_t	err		= NEARDAL_ERROR_NO_ADAPTER;
	GVariant	*tmp		= NULL;
	GVariant	*tmpOut		= NULL;

	NEARDAL_ASSERT_RET(adpArray != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	/* Invoking method 'GetProperties' on Neard Manager */
	if (org_neard_mgr__call_get_properties_sync(neardalMgr.proxy, &tmp,
						    NULL,
					     &neardalMgr.gerror)) {
		NEARDAL_TRACEF("Reading:\n%s\n", g_variant_print(tmp, TRUE));
		NEARDAL_TRACEF("Parsing neard adapters...\n");

		tmpOut = g_variant_lookup_value(tmp, NEARD_MGR_SECTION_ADAPTERS,
						G_VARIANT_TYPE_ARRAY);
		if (tmpOut != NULL) {
			*adpArray = g_variant_dup_objv(tmpOut, len);
			err = NEARDAL_SUCCESS;
		} else
			err = NEARDAL_ERROR_NO_ADAPTER;
	} else {
		err = NEARDAL_ERROR_DBUS_CANNOT_INVOKE_METHOD;
		NEARDAL_TRACE_ERR("%d:%s\n", neardalMgr.gerror->code,
				 neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
	}

	return err;
}


/*****************************************************************************
 * neardal_mgr_prv_get_adapter: Get NFC Adapter from name
 ****************************************************************************/
errorCode_t neardal_mgr_prv_get_adapter(gchar *adpName, AdpProp **adpProp)
{
	errorCode_t	err	= NEARDAL_ERROR_NO_ADAPTER;
	guint		len	= 0;
	AdpProp		*adapter;
	GList		*tmpList;

	tmpList = neardalMgr.prop.adpList;
	while (len < g_list_length(tmpList)) {
		adapter = g_list_nth_data(tmpList, len);
		if (adapter != NULL) {
			if (neardal_tools_prv_cmp_path(adapter->name,
							adpName)) {
				if (adpProp != NULL)
					*adpProp = adapter;
				err = NEARDAL_SUCCESS;
				break;
			}
		}
		len++;
	}

	return err;
}

/*****************************************************************************
 * neardal_mgr_prv_get_adapter_from_proxy: Get NFC Adapter from proxy
 ****************************************************************************/
errorCode_t neardal_mgr_prv_get_adapter_from_proxy(orgNeardAdp *adpProxy,
						   AdpProp **adpProp)
{
	errorCode_t	err	= NEARDAL_ERROR_NO_ADAPTER;
	guint		len = 0;
	AdpProp		*adapter;
	GList		*tmpList;

	NEARDAL_ASSERT_RET(adpProp != NULL, NEARDAL_ERROR_INVALID_PARAMETER);

	tmpList = neardalMgr.prop.adpList;
	while (len < g_list_length(tmpList)) {
		adapter = g_list_nth_data(tmpList, len);
		if (adapter != NULL) {
			if (adapter->proxy == adpProxy) {
				*adpProp = adapter;
				err = NEARDAL_SUCCESS;
				break;
			}
		}
		len++;
	}

	return err;
}

/*****************************************************************************
 * neardal_mgr_create: Get Neard Manager Properties = NFC Adapters list.
 * Create a DBus proxy for the first one NFC adapter if present
 * Register Neard Manager signals ('PropertyChanged')
 ****************************************************************************/
errorCode_t neardal_mgr_create(void)
{
	errorCode_t	err;
	gchar		**adpArray = NULL;
	gsize		adpArrayLen;
	char		*adpName;
	guint		len;

	NEARDAL_TRACEIN();
	if (neardalMgr.proxy != NULL) {
		g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
				G_CALLBACK(neardal_mgr_prv_cb_property_changed),
							NULL);
		g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
				G_CALLBACK(neardal_mgr_prv_cb_adapter_added),
							NULL);
		g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
				G_CALLBACK(neardal_mgr_prv_cb_adapter_removed),
							NULL);
		g_object_unref(neardalMgr.proxy);
		neardalMgr.proxy = NULL;
	}

	neardalMgr.proxy = org_neard_mgr__proxy_new_sync(neardalMgr.conn,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
							NEARD_DBUS_SERVICE,
							NEARD_MGR_PATH,
							NULL, /* GCancellable */
							&neardalMgr.gerror);

	if (neardalMgr.gerror != NULL) {
		NEARDAL_TRACE_ERR(
			"Unable to create Neard Manager Proxy (%d:%s)\n",
				 neardalMgr.gerror->code,
				neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr.gerror);
		return NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY;
	}

	/* Get and store NFC adapters (is present) */
	err = neardal_mgr_prv_get_all_adapters(&adpArray, &adpArrayLen);
	if (adpArray != NULL && adpArrayLen > 0) {
		len = 0;
		while (len < adpArrayLen && err == NEARDAL_SUCCESS) {
			adpName =  adpArray[len++];
			err = neardal_adp_add(adpName);
		}
		g_strfreev(adpArray);
	}

	/* Register for manager signals 'PropertyChanged(String,Variant)' */
	NEARDAL_TRACEF("Register Neard-Manager Signal 'PropertyChanged'\n");
	g_signal_connect(neardalMgr.proxy,
			 NEARD_MGR_SIG_PROPCHANGED,
			 G_CALLBACK(neardal_mgr_prv_cb_property_changed),
			 NULL);

	/* Register for manager signals 'AdapterAdded(ObjectPath)' */
	NEARDAL_TRACEF("Register Neard-Manager Signal 'AdapterAdded'\n");
	g_signal_connect(neardalMgr.proxy,
			 NEARD_MGR_SIG_ADP_ADDED,
			 G_CALLBACK(neardal_mgr_prv_cb_adapter_added),
			 NULL);

	/* Register for manager signals 'AdapterRemoved(ObjectPath)' */
	NEARDAL_TRACEF("Register Neard-Manager Signal 'AdapterRemoved'\n");
	g_signal_connect(neardalMgr.proxy,
			 NEARD_MGR_SIG_ADP_RM,
			 G_CALLBACK(neardal_mgr_prv_cb_adapter_removed),
			 NULL);

	return err;
}

/*****************************************************************************
 * neardal_mgr_destroy: unref DBus proxy, disconnect Neard Manager signals
 ****************************************************************************/
void neardal_mgr_destroy(void)
{
	GList	*node;
	GList	**tmpList;

	NEARDAL_TRACEIN();
	/* Remove all adapters */
	tmpList = &neardalMgr.prop.adpList;
	while (g_list_length((*tmpList))) {
		node = g_list_first((*tmpList));
		neardal_adp_remove(((AdpProp *)node->data));
	}
	neardalMgr.prop.adpList = (*tmpList);

	if (neardalMgr.proxy == NULL)
		return;

	g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
			G_CALLBACK(neardal_mgr_prv_cb_property_changed),
						NULL);
	g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
			G_CALLBACK(neardal_mgr_prv_cb_adapter_added),
						NULL);
	g_signal_handlers_disconnect_by_func(neardalMgr.proxy,
			G_CALLBACK(neardal_mgr_prv_cb_adapter_removed),
						NULL);
	g_object_unref(neardalMgr.proxy);
	neardalMgr.proxy = NULL;
}
