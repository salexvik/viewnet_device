#include "server.h"  // мой заголовочныйный файл
#include "xml.h"


void changePhones(string aData){

    string operationId = xmlGetString(aData, "operationId");
    string phoneId = xmlGetString(aData, "phoneId");


    if (operationId == "addPhone"){
        config = xmlAddTag(config, "phones", xmlGetTag(aData, "phone"));
    }

    if (operationId == "deletePhone"){
        config = xmlDeleteTagById(config, "phone", phoneId);
    }

    if (operationId == "setPhoneName"){
        config = xmlSetStringById(config, "phone", phoneId, "phoneName", xmlGetString(aData, "phoneName"));
    }

    if (operationId == "setPhoneNumber"){
        config = xmlSetStringById(config, "phone", phoneId, "phoneNumber", xmlGetString(aData, "phoneNumber"));
    }

    if (operationId == "setPhoneStatus"){
        config = xmlSetBoolById(config, "phone", phoneId, "phoneStatus", xmlGetBool(aData, "phoneStatus"));
    }


    if (saveConfig()){
        sendClient(xmlStringToTag("id", "changePhonesSuccessfully"));   // отправляяем клиенту сообщение об удачном изменении данных
    }

}
