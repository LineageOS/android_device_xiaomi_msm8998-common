/*
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ril-wrapper"
#define RIL_SHLIB

#include <telephony/ril.h>
#include <log/log.h>

#include <dlfcn.h>

#define RIL_LIB_PATH "/system/vendor/lib64/libril-qc-qmi-1.so"

typedef struct {
	int requestNumber;
	void (*dispatchFunction) (void *p, void *pRI);
	int(*responseFunction) (void *p, void *response, size_t responselen);
} CommandInfo;

typedef struct RequestInfo {
	int32_t token;
	CommandInfo *pCI;
	struct RequestInfo *p_next;
	char cancelled;
	char local;
} RequestInfo;

static const RIL_RadioFunctions *qmiRilFunctions;

static const struct RIL_Env *ossRilEnv;

#define RADIO_TECH_HSPAP_STRING "15"
#define RADIO_TECH_DC_HSPAP_STRING "20"

#define RADIO_TECH_DC_HSPAP 20

static void onRequestCompleteShim(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	RequestInfo *requestInfo;
	int request;

	requestInfo = (RequestInfo *)t;

	if (requestInfo == NULL)
		goto error_null_token;

	request = requestInfo->pCI->requestNumber;

	if (request != RIL_REQUEST_DATA_REGISTRATION_STATE) {
		ALOGI("%s: no need to handle request", __func__);
		goto do_not_handle;
	}

	if (!response) {
		ALOGE("%s: response is NULL", __func__);
		goto do_not_handle;
	}

	/*
	 * RIL reports a rat of 20(DC-HSPAP) which is not supported in AOSP,
	 * resulting in missing mobile data icon.
	 * Change the rat to (15)HSPAP to get a H+ mobile data icon.
	 *
	 * RIL version 15 introduced a RIL_DataRegistrationStateResponse structure,
	 * with a rat member, anything older uses an array of strings,
	 * with the 3rd string being the rat.
	 */
	if (qmiRilFunctions->version < 15) {
		/*
		 * A proper response should contain either 6 or 11 strings,
		 * do not handle other responses.
		 */
		int numStrings = responselen / sizeof(char *);
		if (numStrings != 6 && numStrings != 11) {
			ALOGE("%s: response length is invalid", __func__);
			goto do_not_handle;
		}

		/*
		 * Cast the void pointer to a pointer to an array of strings.
		 */
		char **resp = (char **) response;
		if (resp[3] == NULL) {
			ALOGE("%s: response rat is NULL", __func__);
			goto do_not_handle;
		}

		if (strncmp(resp[3], RADIO_TECH_DC_HSPAP_STRING,
				sizeof(RADIO_TECH_HSPAP_STRING)) == 0) {
			strncpy(resp[3], RADIO_TECH_HSPAP_STRING,
					sizeof(RADIO_TECH_HSPAP_STRING));
			ALOGI("%s: remapping DC-HSPAP to HSPAP", __func__);
		}
	} else {
		if (responselen != sizeof(RIL_DataRegistrationStateResponse)) {
			ALOGE("%s: response length is invalid", __func__);
			goto do_not_handle;
		}

		RIL_DataRegistrationStateResponse *dataRegState =
				(RIL_DataRegistrationStateResponse *)response;

		if (dataRegState->rat == RADIO_TECH_DC_HSPAP) {
			dataRegState->rat = RADIO_TECH_HSPAP;
		}
	}

do_not_handle:
error_null_token:
	ossRilEnv->OnRequestComplete(t, e, response, responselen);
}

const RIL_RadioFunctions* RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
	RIL_RadioFunctions const* (*qmiRilInit)(const struct RIL_Env *env, int argc, char **argv);
	static struct RIL_Env shimmedRilEnv;
	void *qmiRil;

	/*
	 * Save the RilEnv passed from rild.
	 */
	ossRilEnv = env;

	/*
	 * Copy the RilEnv and shim the OnRequestComplete function.
	 */
	shimmedRilEnv = *env;
	shimmedRilEnv.OnRequestComplete = onRequestCompleteShim;

	/* 
	 * Open the real RIL lib.
	 */
	qmiRil = dlopen(RIL_LIB_PATH, RTLD_LOCAL);
	if (!qmiRil) {
		ALOGE("%s: failed to load %s: %s\n", __func__, RIL_LIB_PATH, dlerror());
		return NULL;
	}

	/*
	 * Get a reference to the qmi RIL_Init.
	 */
	qmiRilInit = dlsym(qmiRil, "RIL_Init");
	if (!qmiRilInit) {
		ALOGE("%s: failed to find RIL_Init\n", __func__);
		goto fail_after_dlopen;
	}

	/*
	 * Init the qmi RIL add pass it the shimmed RilEnv.
	 */
	qmiRilFunctions = qmiRilInit(&shimmedRilEnv, argc, argv);
	if (!qmiRilFunctions) {
		ALOGE("%s: failed to get functions from RIL_Init\n", __func__);
		goto fail_after_dlopen;
	}

	return qmiRilFunctions;

fail_after_dlopen:
	dlclose(qmiRil);

	return NULL;
}
