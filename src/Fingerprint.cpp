#include "Fingerprint.h"

#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

namespace internal {
	namespace detail {
		std::string Fingerprint::getBoardSerial()   {

            struct RawSMBIOSData
            {
                BYTE Used20CallingMethod;
                BYTE SMBIOSMajorVersion;
                BYTE SMBIOSMinorVersion;
                BYTE DmiRevision;
                DWORD Length;
                BYTE SMBIOSTableData[];
            };

            struct SMBIOS_Header
            {
                BYTE Type;
                BYTE Length;
                WORD Handle;
            };

                DWORD error = ERROR_SUCCESS;

                DWORD smBiosDataSize = 0;
                RawSMBIOSData* smBiosData = NULL;
                DWORD bytesWritten = 0;
                PBYTE data = NULL;
                BYTE strnum = 0;

                //Запрос размера таблицы
                smBiosDataSize = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

                smBiosData = (RawSMBIOSData*)HeapAlloc(GetProcessHeap(), 0, smBiosDataSize);

                if (!smBiosData) {
                    error = ERROR_OUTOFMEMORY;
                    return "error";
                }
                // Получение таблицы
                bytesWritten = GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);

                if (bytesWritten != smBiosDataSize) {
                    error = ERROR_INVALID_DATA;
                    return "error";
                }
                //data указывает на массив структур SMBIOS
                data = &smBiosData->SMBIOSTableData[0];

                //Нужна структура с типом 0x02.
                //Если тип не 2, то двигаем указатель на длину полей структуры вправо...
                while ( (((SMBIOS_Header*)data)->Type != 2) && (data += ((SMBIOS_Header*)data)->Length))
                {
                    //пропускаем строки
                    while (*(data += (strlen((const char*)data) + 1)));

                    //пропускаем замыкающий нуль
                    data++;
                }

                //номер строки, содержащей серийный номер - по смещению 7 от начала структуры
                strnum = *(data + 7);

                //двигаем вправо в конец полей структуры
                data += ((SMBIOS_Header*)data)->Length;

                //пропускаем все строки перед нужной
                std::string serial;
                for (int i = 1; i < strnum; i++)
                {
                    data += (strlen((const char*)data) + 1);
                }
                std::stringstream a;
                a << data;
                serial = a.str();

                //data указывает на строку с серийником, выводим
//                std::cout << data;

                HeapFree(GetProcessHeap(), 0, smBiosData);
                return serial;

        }

		std::string Fingerprint::getCPUSerial()
        {
            std::string cpuSerial;

            HRESULT hres;

            // Инициализация COM библиотеки
            hres = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(hres))
            {
                std::cerr << "Failed to initialize COM library" << std::endl;
                return "";
            }

            // Инициализация WMI
            hres = CoInitializeSecurity(
                NULL,
                -1,
                NULL,
                NULL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE,
                NULL
            );
            if (FAILED(hres))
            {
                std::cerr << "Failed to initialize security" << std::endl;
                CoUninitialize();
                return "";
            }

            // Инициализация WMI провайдера
            IWbemLocator* pLoc = NULL;
            hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID*)&pLoc
            );
            if (FAILED(hres))
            {
                std::cerr << "Failed to create IWbemLocator object" << std::endl;
                CoUninitialize();
                return "";
            }

            // Подключение к WMI
            IWbemServices* pSvc = NULL;
            hres = pLoc->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"),
                NULL,
                NULL,
                0,
                NULL,
                0,
                0,
                &pSvc
            );
            if (FAILED(hres))
            {
                std::cerr << "Failed to connect to WMI" << std::endl;
                pLoc->Release();
                CoUninitialize();
                return "";
            }

            // Установка безопасности вызова
            hres = CoSetProxyBlanket(
                pSvc,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                NULL,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE
            );
            if (FAILED(hres))
            {
                std::cerr << "Failed to set security blanket" << std::endl;
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return "";
            }

            // Запрос WMI
            IEnumWbemClassObject* pEnumerator = NULL;
            hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM Win32_Processor"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator
            );
            if (FAILED(hres))
            {
                std::cerr << "Failed to execute WMI query" << std::endl;
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return "";
            }

            // Итерация по результатам запроса
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            while (pEnumerator)
            {
                hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (uReturn == 0)
                {
                    break;
                }

                VARIANT vtProp;
                hres = pclsObj->Get(L"ProcessorID", 0, &vtProp, 0, 0);
                if (FAILED(hres))
                {
                    std::cerr << "Failed to get ProcessorID property" << std::endl;
                    pclsObj->Release();
                    continue;
                }

                cpuSerial = _bstr_t(vtProp.bstrVal);

                VariantClear(&vtProp);

                pclsObj->Release();
            }

            // Очистка ресурсов
            pSvc->Release();
            pLoc->Release();
            pEnumerator->Release();
            CoUninitialize();

            return cpuSerial;
        }

		std::string Fingerprint::getMAC()  {

            PIP_ADAPTER_INFO adapterInfo;
            DWORD bufferSize = 0;

            // Получаем размер буфера, необходимого для информации об адаптерах
            GetAdaptersInfo(NULL, &bufferSize);

            adapterInfo = (IP_ADAPTER_INFO *)malloc(bufferSize);

            // Получаем информацию об адаптерах
            std::string info;
            if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
                PIP_ADAPTER_INFO adapter = adapterInfo;

                while (adapter) {

                    char macAddressStr[18];  // Массив для хранения строки MAC-адреса

                    snprintf(macAddressStr, sizeof(macAddressStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                             adapter->Address[0], adapter->Address[1], adapter->Address[2],
                             adapter->Address[3], adapter->Address[4], adapter->Address[5]);
                    info += macAddressStr;

                    adapter = adapter->Next;
                }
            }
            free(adapterInfo);
                return info;
		}
	}
}
