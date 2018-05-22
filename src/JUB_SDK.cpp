#include "JUB_SDK.h"


#include <set>
#include <algorithm>
#include <utility/util.hpp>
#include <token/TokenInterface.hpp>
#include <context/ContextBTC.h>
#include <token/JubiterOneImpl.h>
#include <utility/Singleton.h>
#include <device/JubiterBLEDevice.hpp>
#include <string.h>
#include <utils/logUtils.h>


static std::set<JUB_CHAR_CPTR> memPtrs;



inline JUB_RV _allocMem(JUB_CHAR_PTR_PTR memPtr, const std::string &strBuf) {

    *memPtr = (new char[strBuf.size() + 1]{ 0 });
    if (nullptr == *memPtr) {
        return JUBR_HOST_MEMORY;
    }

    if (0 == strBuf.size()) {
        return JUBR_OK;
    }

    memcpy(*memPtr, strBuf.data(), strBuf.size());

    if (!memPtrs.insert(*memPtr).second) {
        return JUBR_REPEAT_MEMORY_PTR;
    }

    return JUBR_OK;
}


JUB_RV JUB_FreeMemory(IN JUB_CHAR_CPTR memPtr) {
    JUB_CHECK_NULL(memPtr);

    auto pos = memPtrs.find(memPtr);
    if (memPtrs.end() == pos) {
        return JUBR_INVALID_MEMORY_PTR;
    }

    delete[] memPtr;
    memPtr = nullptr;

    memPtrs.erase(pos);
    return JUBR_OK;
}


/*****************************************************************************
* @function name : Jub_ListDeviceHid
* @in param : 
* @out param : 
* @last change : 
*****************************************************************************/

JUB_RV Jub_ListDeviceHid(OUT JUB_UINT16 deviceIDs[MAX_DEVICE])
{
#ifdef HID_MODE

    auto path_list = jub::JubiterHidDevice::enumDevice();

	for (auto path : path_list)
	{
		jub::JubiterOneImpl* token = new jub::JubiterOneImpl(path);
		jub::TokenManager::GetInstance()->addOne(token);
	}

	auto vDeviceIDs = jub::TokenManager::GetInstance()->getHandleList();
	for (JUB_UINT16 i=0 ; i < std::min((size_t)MAX_DEVICE, vDeviceIDs.size()); i++)
	{
		deviceIDs[i] = vDeviceIDs[i];
	}
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif

    return JUBR_OK;
}


JUB_RV Jub_ConnetDeviceHid(IN JUB_UINT16 deviceID)
{
#ifdef HID_MODE
    auto token = jub::TokenManager::GetInstance()->getOne(deviceID);
	if (nullptr != token)
	{
		return token->connectToken();
	}
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
    return JUBR_ERROR;
}

JUB_RV Jub_DisconnetDeviceHid(IN JUB_UINT16 deviceID)
{
#ifdef HID_MODE
    auto token = jub::TokenManager::GetInstance()->getOne(deviceID);
	if (nullptr != token)
	{
		return token->disconnectToken();
	}
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
    return JUBR_ERROR;

}


JUB_RV Jub_CreateContextBTC(IN CONTEXT_CONFIG_BTC cfg, IN JUB_UINT16 deviceID, OUT JUB_UINT16* contextID)
{
    if (nullptr == jub::TokenManager::GetInstance()->getOne(deviceID))
    {
        return JUBR_ERROR;
    }
    jub::ContextBTC* context = new jub::ContextBTC(cfg, deviceID);
    JUB_UINT16 _contextID = jub::ContextManager_BTC::GetInstance()->addOne(context);
    *contextID = _contextID;
    return JUBR_OK;
}


JUB_RV JUB_ClearContext(IN JUB_UINT16 contextID)
{
    jub::ContextManager_BTC::GetInstance()->clearOne(contextID);
    return JUBR_OK;
}


JUB_RV JUB_SignTransactionBTC(IN JUB_UINT16 contextID , IN INPUT_BTC inputs[], IN JUB_UINT16 iCount, IN OUTPUT_BTC outputs[], IN JUB_UINT16 oCount, IN JUB_UINT32 locktime, OUT JUB_CHAR_PTR_PTR raw)
{
    std::vector<INPUT_BTC> vInputs(inputs, inputs + iCount);
    std::vector<OUTPUT_BTC> vOutputs(outputs, outputs + oCount);

    auto context = jub::ContextManager_BTC::GetInstance()->getOne(contextID);
    if (context != nullptr)
    {
        std::string str_raw;
        auto rv = context->signTX(vInputs, vOutputs, locktime, str_raw);
        if (rv == JUBR_OK)
        {
            JUB_VERIFY_RV(_allocMem(raw, str_raw));
            return JUBR_OK;
        }
        return rv;
    }


    return JUBR_ERROR;
}


JUB_RV JUB_ShowVirtualPwd(IN JUB_UINT16 contextID)
{
    auto context = jub::ContextManager_BTC::GetInstance()->getOne(contextID);
    if (context != nullptr)
    {
        return context->showVirtualPwd();
    }

    return JUBR_ERROR;
}


JUB_RV JUB_VerifyPIN(IN JUB_UINT16 contextID, IN JUB_CHAR_PTR pinMix, OUT JUB_ULONG &retry)
{
    auto context = jub::ContextManager_BTC::GetInstance()->getOne(contextID);
    if (context != nullptr)
    {
        return context->verifyPIN(pinMix, retry);
    }

    return JUBR_ERROR;
}


JUB_RV JUB_GetHDNodeBTC(IN JUB_UINT16 contextID, IN JUB_UINT64	nodeIndex, OUT JUB_CHAR_PTR_PTR xpub)
{
    auto context = jub::ContextManager_BTC::GetInstance()->getOne(contextID);
    JUB_CHECK_NULL(context);
    std::string str_xpub;
    JUB_VERIFY_RV(context->getHDNode(nodeIndex, str_xpub));
    JUB_VERIFY_RV(_allocMem(xpub, str_xpub));
    return JUBR_OK;

}

JUB_RV JUB_GetAddressBTC(IN JUB_UINT16 contextID, IN JUB_UINT64 addressIndex, IN JUB_ENUM_BOOL bshow, OUT JUB_CHAR_PTR_PTR address)
{
    auto context = jub::ContextManager_BTC::GetInstance()->getOne(contextID);
    JUB_CHECK_NULL(context);
    std::string str_address;
    JUB_VERIFY_RV(context->getAddres(addressIndex, bshow, str_address));
    JUB_VERIFY_RV(_allocMem(address, str_address));
    return JUBR_OK;
}


JUB_RV Jub_GetDeviceInfo(IN JUB_UINT16 deviceID, OUT JUB_DEVICE_INFO& info)
{
    auto token = jub::TokenManager::GetInstance()->getOne(deviceID);
    JUB_CHECK_NULL(token);
    /*
    JUB_VERIFY_RV(token->getPinRetry(info.pin_retry));
    JUB_VERIFY_RV(token->getPinMaxRetry(info.pin_max_retry));
    JUB_VERIFY_RV(token->getSN(sn));
    JUB_VERIFY_RV(token->getLabel(label));*/

	//选主安全域，不需要判断返回值，用来拿到后面的数据
	token->isBootLoader();

    JUB_BYTE sn[24] = { 0 };
    JUB_BYTE label[32] = { 0 };
    JUB_BYTE retry = 0;
    JUB_BYTE max_retry = 0;
    JUB_BYTE ble_version[4] = { 0 };
    JUB_BYTE fw_version[4] = { 0 };

    token->getPinRetry(retry);
    token->getPinMaxRetry(max_retry);
    token->getSN(sn);
    token->getLabel(label);
    token->getBleVersion(ble_version);
    token->getFwVersion(fw_version);


    memcpy(info.sn, sn, 24);
    memcpy(info.label, label, 32);
    info.pin_retry = retry;
    info.pin_max_retry = max_retry;
    memcpy(info.ble_version, ble_version, 4);
    memcpy(info.firmware_version, fw_version, 4);

    return JUBR_OK;
}

JUB_ENUM_BOOL JUB_IsInitialize(IN JUB_UINT16 deviceID)
{
    auto token = jub::TokenManager::GetInstance()->getOne(deviceID);
    if (token == nullptr)
        return BOOL_FALSE;
    return (JUB_ENUM_BOOL)token->isInitialize();

}

JUB_ENUM_BOOL JUB_IsBootLoader(IN JUB_UINT16 deviceID)
{
    auto token = jub::TokenManager::GetInstance()->getOne(deviceID);
    if (token == nullptr)
        return BOOL_FALSE;
    return (JUB_ENUM_BOOL)token->isBootLoader();
}


JUB_RV JUB_EnumApplets()
{
    return JUBR_IMPL_NOT_SUPPORT;
}

JUB_RV JUB_CurrentAppletID()
{
    return JUBR_IMPL_NOT_SUPPORT;
}

JUB_RV JUB_SelectApplet()
{
    return JUBR_IMPL_NOT_SUPPORT;
}


/// ble device APIs //////////////////////////////////////////
// only works in ble (android and ios)

//#if defined(ANDROID) || defined(TARGET_OS_IPHONE)


using BLE_device_map = Singleton<xManager<JUB_ULONG>>;


JUB_RV JUB_initDevice(IN DEVICE_INIT_PARAM param) {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();
    if (!bleDevice) {
        return JUBR_ERROR;
    }

    return bleDevice->initialize(
        { param.param, param.callBack, param.scanCallBack, param.discCallBack });

#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_enumDevices() {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();

    if (!bleDevice) {
        return JUBR_ERROR;
    }

    return bleDevice->scan();

#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_stopEnumDevices() {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();
    if (!bleDevice) {
        return JUBR_ERROR;
    }

    return bleDevice->stopScan();
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_connectDevice(JUB_BYTE_PTR bBLEUUID, JUB_UINT32 connectType,
                         JUB_UINT16* pDevice_ID, JUB_UINT32 timeout) {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();

    if (!bleDevice) {
        return JUBR_ERROR;
    }
    JUB_ULONG * pDevHandle = new JUB_ULONG;

    JUB_RV rv = bleDevice->connect(bBLEUUID, connectType, pDevHandle, timeout);

    LOG_INF("JUB_connectDevice rv: %d", *pDevHandle);
    if (JUBR_OK == rv)
    {
        *pDevice_ID = BLE_device_map::GetInstance()->addOne(pDevHandle);
        LOG_INF("JUB_connectDevice rv: %d", pDevice_ID);
        jub::JubiterOneImpl* token = new jub::JubiterOneImpl(bleDevice);
        jub::TokenManager::GetInstance()->addOne(*pDevice_ID, token);
    }

    return rv;

#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_cancelConnect(JUB_BYTE_PTR bBLEUUID) {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();

    if (!bleDevice) {
        return JUBR_ERROR;
    }

    return bleDevice->cancelConnect(bBLEUUID);
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_disconnectDevice(JUB_UINT16 deviceID) {
#ifdef BLE_MODE
    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();

    if (!bleDevice) {
        return JUBR_ERROR;
    }

    JUB_ULONG *devHandle =  BLE_device_map::GetInstance()->getOne(deviceID);

    return bleDevice->disconnect(*devHandle);
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}

JUB_RV JUB_isDeviceConnect(JUB_UINT16 deviceID) {
#ifdef BLE_MODE

    auto bleDevice = Singleton<jub::JubiterBLEDevice>::GetInstance();

    if (!bleDevice) {
        return JUBR_ERROR;
    }
    JUB_ULONG *devHandle = BLE_device_map::GetInstance()->getOne(deviceID);
    if (devHandle == NULL) {
        return JUBR_CONNECT_DEVICE_ERROR;
    }

    return bleDevice->isConnect(*devHandle);
#else
    return JUBR_IMPL_NOT_SUPPORT;
#endif
}
//#endif // #if defined(ANDROID) || defined(TARGET_OS_IPHONE)


