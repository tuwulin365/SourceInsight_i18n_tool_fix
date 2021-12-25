#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int iCurrentLine;
    int iInsertCnt;
    unsigned char ucInsertStr[4096];
    int iInsertStrLen;
    unsigned char ucInsertAddr[4096];
    int iInsertAddrStrLen;
}StringPair;

unsigned char* GetStringLine(unsigned char *pBuf, int iInLen, unsigned char *pOut, int *iOutLen, int *iCurrentLine)
{
    int i;

    for (i = 0; i < iInLen; i++)
    {
        if ((pBuf[i] == 0x0D) && (pBuf[i+1] == 0x0A))
        {
            break;
        }
    }

    if (i < iInLen)
    {
        *iCurrentLine += 1;
        if (0 == memcmp(pBuf, "//", 2))
        {
            return GetStringLine(pBuf+i+2, iInLen-i-2, pOut, iOutLen, iCurrentLine);
        }
        else
        {
            memcpy(pOut, pBuf, i);
            *iOutLen = i;
            return pBuf+i+2;
        }
    }

    return NULL;
}

unsigned char* GetStringPair(unsigned char *pBuf, int iInLen, StringPair *pstString)
{
    unsigned char *pTmp;
    unsigned char uczBuf[1024];
    int i;

    memset(pstString->ucInsertStr, 0, sizeof(pstString->ucInsertStr));
    pstString->iInsertStrLen = 0;
    memset(pstString->ucInsertAddr, 0, sizeof(pstString->ucInsertAddr));
    pstString->iInsertAddrStrLen = 0;

    pTmp = GetStringLine(pBuf, iInLen, pstString->ucInsertStr, &pstString->iInsertStrLen, &pstString->iCurrentLine);
    if (pTmp)
    {
        pTmp = GetStringLine(pTmp, iInLen-(pTmp-pBuf), pstString->ucInsertAddr, &pstString->iInsertAddrStrLen, &pstString->iCurrentLine);
        if (pTmp)
        {
            pTmp = GetStringLine(pTmp, iInLen-(pTmp-pBuf), uczBuf, &i, &pstString->iCurrentLine);
        }
    }

    return pTmp ? pTmp : NULL;
}

//2c0000  008E6F00
#define EXE_STR_INSERT_START_ADDR   0x002C0000
#define EXE_CODE_START              0x006F8E00
int FixExe(unsigned char *pExeBuf, StringPair *pStringPair)
{
    int iInsertCnt;
    unsigned int uInsertAddr[100];
    char *pTmp;
    unsigned int uTmp;

    pTmp = (char*)pStringPair->ucInsertAddr;
    for (iInsertCnt = 0; pTmp; iInsertCnt++)
    {
        sscanf(pTmp, "%x", &uInsertAddr[iInsertCnt]);
        pTmp = strchr(pTmp, ',');
        if (pTmp)
        {
            pTmp++;
        }
    }

    for (int i = 0; i < iInsertCnt; i++)
    {
        printf("0x%x ", uInsertAddr[i]);
    }
    printf("\n\n");

    memcpy(pExeBuf + EXE_STR_INSERT_START_ADDR + pStringPair->iInsertCnt * 16, pStringPair->ucInsertStr, pStringPair->iInsertStrLen);
    uTmp = EXE_CODE_START + pStringPair->iInsertCnt * 16;
    for (int i = 0; i < iInsertCnt; i++)
    {
        memcpy(pExeBuf + uInsertAddr[i],
               (void*)&uTmp, 4);
    }

    pStringPair->iInsertCnt++;

    return 0;
}

int main(int iCnt, char *pParam[])
{
    FILE *pfLngFile;
    FILE *pfExe;
    unsigned char *pucLng;
    unsigned char *pucExe;
    StringPair stString;
    int i,j;
    unsigned char *pTmpIn;
    unsigned char *pTmpOut;

    char czNewFileName[128+8];
    char czOldFileName[128];

    printf("sourceinsight4 i18n fix tool V1.00\ntuwulin365@126.com  2021-05-27\n");
    printf("usage: i18n_fix.exe si.exe str_list.lng\n\n");

    if (iCnt != 3)
    {
        printf("param err.\n");
        return -1;
    }

    pfLngFile = fopen(pParam[2], "rb");
    if (!pfLngFile)
    {
        printf("open lng file error: %s.\n", pParam[2]);
        return -1;
    }

    pfExe = fopen(pParam[1], "rb");
    if (!pfExe)
    {
        printf("open org exe file error: %s.\n", pParam[1]);
        return -1;
    }

    pucLng = malloc(1*1024*1024);
    pucExe = malloc(5*1024*1024);
    if (!pucExe || !pucLng)
    {
        printf("malloc error.\n");
        return -1;
    }

    i = fread(pucLng, 1, 1*1024*1024, pfLngFile);
    j = fread(pucExe, 1, 5*1024*1024, pfExe);
    fclose(pfLngFile);
    fclose(pfExe);

    memset(&stString, 0, sizeof(StringPair));
    pTmpIn = pucLng;
    pTmpOut = pTmpIn;
    while (pTmpOut)
    {
        pTmpOut = GetStringPair(pTmpIn, i, &stString);
        if (pTmpOut)
        {
            i = i - (pTmpOut-pTmpIn);
            pTmpIn = pTmpOut;
            printf("%s %d\n", stString.ucInsertStr, stString.iInsertStrLen);
            printf("%s %d\n", stString.ucInsertAddr, stString.iInsertAddrStrLen);

            if (FixExe(pucExe, &stString))
            {
                printf("fix error!\n");
                exit(-2);
            }
        }
    }

    strcpy(czOldFileName, pParam[1]);
    czOldFileName[strlen(czOldFileName)-4] = 0;

    sprintf(czNewFileName, "%s_fix.exe", czOldFileName);
    pfExe = fopen(czNewFileName, "wb");
    if (!pfExe)
    {
        printf("new file error.\n");
        return -1;
    }

    fwrite(pucExe, 1, j, pfExe);
    fclose(pfExe);

    printf("DONE !\n");
    return 0;
}
