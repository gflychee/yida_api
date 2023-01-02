#include "ydExample.h"

class YDExample1Listener: public YDListener
{
private:
	YDApi *m_pApi;
	const char *m_username,*m_password,*m_instrumentID;
	int m_maxPosition;
	int m_maxOrderRef;

	// Ö¸ÏòÏ£Íû½»Ò×µÄÆ·ÖÖ
	const YDInstrument *m_pInstrument;

	// Ö¸ÏòÏ£Íû½»Ò×µÄÆ·ÖÖµÄÕË»§Æ·ÖÖÐÅÏ¢
	const YDAccountInstrumentInfo *m_pAccountInstrumentInfo;

	// ËµÃ÷µ±Ç°ÊÇ·ñÓÐ¹Òµ¥
	bool m_hasOrder;
public:
	YDExample1Listener(YDApi *pApi,const char *username,const char *password,const char *instrumentID,int maxPosition)
	{
		m_pApi=pApi;
		m_username=username;
		m_password=password;
		m_instrumentID=instrumentID;
		m_maxPosition=maxPosition;
		m_maxOrderRef=0;
		m_hasOrder=false;
	}
	virtual void notifyReadyForLogin(bool hasLoginFailed)
	{
		// µ±API×¼±¸µÇÂ¼Ê±£¬·¢³ö´ËÏûÏ¢£¬ÓÃ»§ÐèÒªÔÚ´ËÊ±·¢³öµÇÂ¼Ö¸Áî
		// Èç¹û·¢ÉúÁË¶ÏÏßÖØÁ¬£¬Ò²»á·¢³ö´ËÏûÏ¢£¬ÈÃÓÃ»§ÖØÐÂ·¢³öµÇÂ¼Ö¸Áî£¬µ«ÊÇ´ËÊ±²»ÔÊÐí»»³ÉÁíÍâµÄÓÃ»§µÇÂ¼
		if (!m_pApi->login(m_username,m_password,NULL,NULL))
		{
			printf("can not login\n");
			exit(1);
		}
	}
	virtual void notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
	{
		// Ã¿´ÎµÇÂ¼ÏìÓ¦£¬¶¼»á»ñµÃ´ËÏûÏ¢£¬ÓÃ»§Ó¦µ±¸ù¾ÝerrorNoÀ´ÅÐ¶ÏÊÇ·ñµÇÂ¼³É¹¦
		if (errorNo==0)
		{
			// µÇÂ¼³É¹¦ºó£¬Ó¦µ±¼ÇÂ¼µ±Ç°µÄ×î´ó±¨µ¥ÒýÓÃ£¬ÔÚ±¨µ¥Ê±ÓÃ¸ü´óµÄÊý×÷Îª±¨µ¥ÒýÓÃ£¬ÒÔ±ã³ÌÐòÍ¨¹ý±¨µ¥ÒýÓÃÀ´Ê¶±ð±¨µ¥
			m_maxOrderRef=maxOrderRef;
			printf("login successfully\n");
		}
		else
		{
			// Èç¹ûµÇÂ¼Ê§°Ü£¬ÓÐ¿ÉÄÜÊÇ·þÎñÆ÷¶ËÉÐÎ´Æô¶¯£¬ËùÒÔ¿ÉÒÔÑ¡Ôñ²»ÖÕÖ¹³ÌÐò£¬µ«ÊÇ²»ÐèÒªÔÚÕâÀïÔÙ´Î·¢³öµÇÂ¼ÇëÇó¡£
			// Api»áÉÔ¹ýÒ»»á¶ùÔÙ´Î¸ø³önotifyReadyForLoginÏûÏ¢£¬Ó¦µ±ÔÚÄÇÊ±·¢³öµÇÂ¼ÇëÇó
			printf("login failed, errorNo=%d\n",errorNo);
			exit(1);
		}
	}
	virtual void notifyFinishInit(void)
	{
		// notifyFinishInitÊÇÔÚµÚÒ»´ÎµÇÂ¼³É¹¦ºóÒ»Ð¡¶ÎÊ±¼ä·¢³öµÄÏûÏ¢£¬±íÊ¾APIÒÑ¾­ÊÕµ½ÁË½ñÌìµÄËùÓÐ³õÊ¼»¯ÐÅÏ¢£¬
		// °üÀ¨ËùÓÐµÄ²úÆ·ºÏÔ¼ÐÅÏ¢£¬×òÐÐÇéºÍ½ñÈÕµÄ¹Ì¶¨ÐÐÇé£¨ÀýÈçÕÇµøÍ£°å£©ÐÅÏ¢£¬ÕËºÅµÄÈÕ³õÐÅÏ¢£¬±£Ö¤½ðÂÊÐÅÏ¢£¬
		// ÊÖÐø·ÑÂÊÐÅÏ¢£¬×ò³Ö²ÖÐÅÏ¢£¬µ«ÊÇ»¹Ã»ÓÐ»ñµÃµÇÂ¼Ç°ÒÑ¾­·¢ÉúµÄ±¨µ¥ºÍ³É½»ÐÅÏ¢£¬ÈÕÄÚµÄ³öÈë½ðÐÅÏ¢
		// Õâ¸öÊ±ºò£¬ÓÃ»§³ÌÐòÒÑ¾­¿ÉÒÔ°²È«µØ·ÃÎÊËùÓÐAPI¹ÜÀíµÄÊý¾Ý½á¹¹ÁË
		// ÓÃ»§³ÌÐò»ñµÃËùÓÐYDSystemParam£¬YDExchange£¬YDProduct£¬YDInstrument£¬YDCombPositionDef£¬YDAccount£¬
		// YDPrePosition£¬YDMarginRate£¬YDCommissionRate£¬YDAccountExchangeInfo£¬YDAccountProductInfo£¬
		// YDAccountInstrumentInfoºÍYDMarketDataµÄÖ¸Õë£¬¶¼¿ÉÒÔÔÚÎ´À´³¤ÆÚ°²È«Ê¹ÓÃ£¬API²»»á¸Ä±äÆäµØÖ·
		// µ«ÊÇAPIÏûÏ¢ÖÐµÄYDOrder¡¢YDTrade¡¢YDInputOrder¡¢YDQuote¡¢YDInputQuoteºÍYDCombPositionµÄµØÖ·¶¼ÊÇ
		// ÁÙÊ±µÄ£¬ÔÚ¸ÃÏûÏ¢´¦ÀíÍê³Éºó½«²»ÔÙÓÐÐ§
		m_pInstrument=m_pApi->getInstrumentByID(m_instrumentID);
		if (m_pInstrument==NULL)
		{
			printf("can not find instrument %s\n",m_instrumentID);
			exit(1);
		}
		m_pAccountInstrumentInfo=m_pApi->getAccountInstrumentInfo(m_pInstrument);
		// ÏÂÃæÕâ¸öÑ­»·Õ¹Ê¾ÁËÈçºÎ¸ù¾ÝÀúÊ·³Ö²ÖÐÅÏ¢£¬¼ÆËã¸ÃÆ·ÖÖµÄµ±Ç°³Ö²Ö¡£Èç¹ûÓÃ»§·ç¿ØºöÂÔÀúÊ·³Ö²Ö£¬¿ÉÒÔ²»Ê¹ÓÃ
		for (int i=0;i<m_pApi->getPrePositionCount();i++)
		{
			const YDPrePosition *pPrePosition=m_pApi->getPrePosition(i);
			if (pPrePosition->m_pInstrument==m_pInstrument)
			{
				if (pPrePosition->PositionDirection==YD_PD_Long)
				{
					// ËùÓÐ¸÷¸ö½á¹¹ÖÐµÄUserInt1£¬UserInt2£¬UserFloat£¬pUser¶¼ÊÇ¹©ÓÃ»§×ÔÓÉÊ¹ÓÃµÄ£¬Æä³õÖµ¶¼ÊÇ0
					// ÔÚ±¾Àý×ÓÖÐ£¬ÎÒÃÇÊ¹ÓÃÕË»§ºÏÔ¼ÐÅÏ¢ÖÐµÄUserInt1±£´æµ±Ç°µÄ³Ö²ÖÐÅÏ¢
					m_pAccountInstrumentInfo->UserInt1+=pPrePosition->PrePosition;
				}
				else
				{
					m_pAccountInstrumentInfo->UserInt1-=pPrePosition->PrePosition;
				}
			}
		}
		printf("Position=%d\n",m_pAccountInstrumentInfo->UserInt1);
		m_pApi->subscribe(m_pInstrument);
	}
	virtual void notifyMarketData(const YDMarketData *pMarketData)
	{
		if (m_pInstrument->m_pMarketData!=pMarketData)
		{
			// ÓÉÓÚ¸÷¸öÆ·ÖÖµÄpMarketDataµÄµØÖ·ÊÇ¹Ì¶¨µÄ£¬ËùÒÔ¿ÉÒÔÓÃ´Ë·½·¨ºöÂÔ·Ç±¾Æ·ÖÖµÄÐÐÇé
			return;
		}
		if ((pMarketData->AskVolume==0)||(pMarketData->BidVolume==0))
		{
			// ºöÂÔÍ£°åÐÐÇé
			return;
		}
		if (pMarketData->BidVolume-pMarketData->AskVolume>100)
		{
			// ¸ù¾Ý²ßÂÔÌõ¼þ£¬ÐèÒªÂòÈë
			tryTrade(YD_D_Buy);
		}
		else if (pMarketData->AskVolume-pMarketData->BidVolume>100)
		{
			// ¸ù¾Ý²ßÂÔÌõ¼þ£¬ÐèÒªÂô³ö
			tryTrade(YD_D_Sell);
		}
	}
	void tryTrade(int direction)
	{
		if (m_hasOrder)
		{
			// Èç¹ûÓÐ¹Òµ¥£¬¾Í²»×öÁË
			return;
		}
		YDInputOrder inputOrder;
		// inputOrderÖÐµÄËùÓÐ²»ÓÃµÄ×Ö¶Î£¬Ó¦µ±Í³Ò»Çå0
		memset(&inputOrder,0,sizeof(inputOrder));
		if (direction==YD_D_Buy)
		{
			if (m_pAccountInstrumentInfo->UserInt1>=m_maxPosition)
			{
				// ¿ØÖÆÊÇ·ñ´ïµ½ÁËÏÞ²Ö
				return;
			}
			if (m_pAccountInstrumentInfo->UserInt1>=0)
			{
				// ¿ØÖÆ¿ªÆ½²Ö£¬Õâ¸öÀý×ÓÖÐÃ»ÓÐ´¦ÀíSHFEºÍINEµÄÇø·Ö½ñ×ò²ÖµÄÇé¿ö
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			// ÓÉÓÚ±¾Àý×ÓÊ¹ÓÃµÄ²»ÊÇÊÐ¼Ûµ¥£¬ËùÒÔÐèÒªÖ¸¶¨¼Û¸ñ
			inputOrder.Price=m_pInstrument->m_pMarketData->AskPrice;
		}
		else
		{
			if (m_pAccountInstrumentInfo->UserInt1<=-m_maxPosition)
			{
				return;
			}
			if (m_pAccountInstrumentInfo->UserInt1<=0)
			{
				inputOrder.OffsetFlag=YD_OF_Open;
			}
			else
			{
				inputOrder.OffsetFlag=YD_OF_Close;
			}
			inputOrder.Price=m_pInstrument->m_pMarketData->BidPrice;
		}
		inputOrder.Direction=direction;
		inputOrder.HedgeFlag=YD_HF_Speculation;
		inputOrder.OrderVolume=1;
		// Ê¹ÓÃÏÂÒ»¸öÏÂÒ»¸ö±¨µ¥ÒýÓÃ¡£YD·þÎñÆ÷²»¼ì²éOrderRef£¬Ö»ÊÇ½«ÆäÓÃÓÚÔÚ±¨µ¥ºÍ³É½»»Ø±¨ÖÐ·µ»Ø
		// ÓÃ»§¿ÉÒÔ×ÔÐÐÑ¡ÔñOrderRefµÄ±àÂë·½Ê½
		// ¶ÔÓÚ·Ç±¾ÏµÍ³±¾´ÎÔËÐÐ²úÉúµÄ±¨µ¥£¬ÏµÍ³·µ»ØµÄOrderRefÒ»ÂÉÊÇ-1
		// YDClient²úÉúµÄ±¨µ¥£¬OrderRefÒ»ÂÉÊÇ0
		inputOrder.OrderRef=++m_maxOrderRef;
		// Õâ¸öÀý×ÓÊ¹ÓÃÏÞ¼Ûµ¥
		inputOrder.OrderType=YD_ODT_Limit;
		// ËµÃ÷ÊÇÆÕÍ¨±¨µ¥
		inputOrder.YDOrderFlag=YD_YOF_Normal;
		// ËµÃ÷ÈçºÎÑ¡ÔñÁ¬½Ó
		inputOrder.ConnectionSelectionType=YD_CS_Any;
		// Èç¹ûConnectionSelectionType²»ÊÇYD_CS_Any£¬ÐèÒªÖ¸¶¨ConnectionID£¬·¶Î§ÊÇ0µ½¶ÔÓ¦µÄYDExchangeÖÐµÄConnectionCount-1
		inputOrder.ConnectionID=0;
		// inputOrderÖÐµÄRealConnectionIDºÍErrorNoÊÇÔÚ·µ»ØÊ±ÓÉ·þÎñÆ÷ÌîÐ´µÄ
		if (m_pApi->insertOrder(&inputOrder,m_pInstrument))
		{
			m_hasOrder=true;
		}
	}
	virtual void notifyOrder(const YDOrder *pOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pOrder->OrderStatus==YD_OS_Queuing)
		{
			// Èç¹û»¹ÔÚ¹Òµ¥£¬¾ÍÖ±½Ó³·µ¥
			// Èç¹ûinputOrderµÄOrderType²»ÊÇYD_ODT_Limit£¬ÄÇ¾Í²»ÐèÒªÕâ¶Î³·µ¥Âß¼­
			YDCancelOrder cancelOrder;
			memset(&cancelOrder,0,sizeof(cancelOrder));
			cancelOrder.OrderSysID=pOrder->OrderSysID;
			m_pApi->cancelOrder(&cancelOrder,pInstrument->m_pExchange,pAccount);
		}
		else
		{
			m_hasOrder=false;
		}
	}
	virtual void notifyTrade(const YDTrade *pTrade, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		if (pTrade->Direction==YD_D_Buy)
		{
			// ¸ù¾Ý³É½»£¬µ÷Õû³Ö²Ö
			m_pAccountInstrumentInfo->UserInt1+=pTrade->Volume;
		}
		else
		{
			m_pAccountInstrumentInfo->UserInt1-=pTrade->Volume;
		}
		printf("%s %s %d at %g\n",(pTrade->Direction==YD_D_Buy?"buy":"sell"),(pTrade->OffsetFlag==YD_OF_Open?"open":"close"),pTrade->Volume,pTrade->Price);
		printf("Position=%d\n",m_pAccountInstrumentInfo->UserInt1);
	}
	virtual void notifyFailedOrder(const YDInputOrder *pFailedOrder, const YDInstrument *pInstrument, const YDAccount *pAccount)
	{
		// ±¨µ¥Ê§°ÜµÄ´¦Àí
		// ×¢Òâ£¬ÓÐÐ©±¨µ¥Ê§°ÜÊÇÍ¨¹ýnotifyOrder·µ»ØµÄ£¬´ËÊ±OrderStatusÒ»¶¨ÊÇYD_OS_Rejected£¬ÇÒErrorNoÖÐÓÐ·Ç0´íÎóºÅ
		m_hasOrder=false;
	}
};

void startExample1(const char *configFilename,const char *username,const char *password,const char *instrumentID)
{
	/// ËùÓÐYDApiµÄÆô¶¯¶¼ÓÉÏÂÁÐÈý²½¹¹³É

	// ´´½¨YDApi
	YDApi *pApi=makeYDApi(configFilename);
	if (pApi==NULL)
	{
		printf("can not create API\n");
		exit(1);
	}
	// ´´½¨ApiµÄ¼àÌýÆ÷
	YDExample1Listener *pListener=new YDExample1Listener(pApi,username,password,instrumentID,3);
	/// Æô¶¯Api
	if (!pApi->start(pListener))
	{
		printf("can not start API\n");
		exit(1);
	}
}
