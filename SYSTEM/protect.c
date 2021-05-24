#include "stdint.h"
#include "stm32f10x_flash.h"
#include "main.h"

/****************************************************************
 * Function:    Flash_EnableReadProtection
 * Description: Enable the read protection of user flash area.
 * Input:
 * Output:
 * Return:      1: Read Protection successfully enable
 *              2: Error: Flash read unprotection failed
*****************************************************************/
uint32_t Flash_EnableReadProtection(void)
{
	if(FLASH_GetReadOutProtectionStatus() != SET)
	{
//		FLASH_Unlock();//������FALSH Ҳ�����ö�����������
		FLASH_ReadOutProtection(ENABLE);
//		FLASH_Lock();//����
		return TRUE;
	}
	return FALSE;
}
 
/****************************************************************
 * Function:    Flash_DisableReadProtection
 * Description: Disable the read protection of user flash area.
 * Input:
 * Output:
 * Return:      1: Read Protection successfully disable
 *              2: Error: Flash read unprotection failed
*****************************************************************/
uint32_t Flash_DisableReadProtection(void)
{
	if(FLASH_GetReadOutProtectionStatus() != RESET)
	{
		FLASH_Unlock();
		FLASH_ReadOutProtection(DISABLE);
		FLASH_Lock();//����
		return TRUE;
	}
	return FALSE;
}

