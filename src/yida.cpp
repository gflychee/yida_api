#include "ydApi.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <list>
#include <fstream>


using namespace std;


string YIDA_CONFIG="";
string USERID = "";
string APPID = "";
string AUTHCODE = "";
string PASSWORD = "";

TraderYida *yida = NULL

class TraderYida : public YDListener {
public:
	TraderYida();
	int login(const char *configFilename);
	int logout();
	inline int send_order(struct order *order);
	inline int cancel_order(struct order *order);
	YDInputOrder new_order;
	YDCancelOrder cancel;
	YDApi *api;
private:
	virtual void notifyFinishInit(void);
	virtual void notifyReadyForLogin(bool is_failed);
	virtual void notifyLogin(int errid, int orderref, bool is_monitor);
	virtual void notifyOrder(const YDOrder *order, const YDInstrument *instrument, const YDAccount *account);
	virtual void notifyTrade(const YDTrade *trade, const YDInstrument *instrument, const YDAccount *account);
	virtual void notifyFailedOrder(const YDInputOrder *order, const YDInstrument *instrument, const YDAccount *account);
	virtual void notifyFailedCancelOrder(const YDFailedCancelOrder *rtn,const YDExchange *pExchange,const YDAccount *pAccount);
	virtual void notifyCaughtUp(void);
	virtual void notifyAfterApiDestroy(void);

	struct TraderYidaAccount {
		const char *config;
		const char *userid;
		const char *passwd;
		const char *authcode;
		const char *appid;
	} acc;



	map<int, int> id2ref;
	map<int, int> ref2id;

	int orderref_init;
	int orderref;
	const YDInstrument *instrument[MAX_INSTRUMENT_NR];
	volatile int login_finished;
	volatile int logout_finished;
};

TraderYida::TraderYida()
{
	acc.config = YIDA_CONFIG.c_str();
	acc.userid = USERID.c_str();
	acc.passwd = PASSWORD.c_str();
	acc.authcode = AUTHCODE.c_str();
	acc.appid = APPID.c_str();
	memset(&new_order, 0, sizeof(new_order));
	memset(&cancel, 0, sizeof(cancel));
}

int TraderYida::login(const char *configFilename)
{
	login_finished = 0;
	api = makeYDApi(configFilename);
	api->start(this);

	while (!login_finished)
		usleep(1000);

	return 0;
}

int TraderYida::logout()
{
	logout_finished = 0;
	printf("try logout\n");
	api->startDestroy();

	while (!logout_finished)
		usleep(1000);

	return 0;
}

void TraderYida::notifyReadyForLogin(bool hasLoginFailed)
{
	printf("try login %s\n", acc.userid);

	if (!api->login(acc.userid, acc.passwd, acc.appid, acc.authcode)) {
		printf("cannot login: hasLoginFailed = %d, errno = %d\n", hasLoginFailed, errno);
		fflush(stdout);
		exit(1);
	}
}

void TraderYida::notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
{
	if (errorNo == 0) {
		orderref = orderref_init = maxOrderRef;
		printf("login successfully %d\n", orderref);
	} else {
		printf("login failed, errorNo=%d\n", errorNo);
		exit(1);
	}
}

void TraderYida::notifyCaughtUp()
{
	login_finished = 1;
}

void TraderYida::notifyAfterApiDestroy()
{
	printf("logout successfully\n");
	logout_finished = 1;
}

void TraderYida::notifyFinishInit(void)
{
	int i = 0;
	const char *insid = idx2ins(trader.instab, i);

	while (insid[0]) {
		instrument[i] = api->getInstrumentByID(insid);
		insid = idx2ins(trader.instab, i);
	}
}

static inline char encode_direction(struct orderflags flags)
{
	const char ret[] = {YD_D_Buy, YD_D_Sell};

	return ret[flags.direction];
}

static inline int encode_offset(struct orderflags flags)
{
	const int ret[] = {YD_OF_Open, YD_OF_CloseToday, YD_OF_CloseYesterday};

	return ret[flags.offset];
}

static inline int encode_hedge(struct orderflags flags)
{
	const int ret[] = {YD_HF_Speculation, YD_HF_Hedge};

	return ret[flags.hedge];
}

static inline int encode_timecond(struct orderflags flags)
{
	const int ret[] = {YD_ODT_Limit, YD_ODT_FAK};

	return ret[flags.timecond];
}

int TraderYida::send_order(struct order *order)
{
	new_order.Direction = encode_direction(order->flags);
	new_order.OffsetFlag = encode_offset(order->flags);
	new_order.HedgeFlag = encode_hedge(order->flags);
	new_order.Price = order->price;
	new_order.OrderVolume = order->volume;
	new_order.OrderRef = ++orderref;
	new_order.OrderType = encode_timecond(order->flags);

	if (order->branchid != -1) {
		new_order.ConnectionSelectionType = YD_CS_Fixed;
		new_order.ConnectionID = order->branchid;
	} else {
		new_order.ConnectionSelectionType = YD_CS_Any;
	}

	int ret = api->insertOrder(&new_order, instrument[order->insidx]);

	id2ref[order->orderid] = new_order.OrderRef;
	ref2id[new_order.OrderRef] = order->orderid;

	return ret;
}

int TraderYida::cancel_order(struct order *order)
{
	cancel.OrderSysID = order->sysid;

	int branchid = trader_get_branchid(&trader);

	if (branchid != -1) {
		cancel.ConnectionSelectionType = YD_CS_Prefered;
		cancel.ConnectionID = branchid;
	} else {
		cancel.ConnectionSelectionType = YD_CS_Any;
	}

	return api->cancelOrder(&cancel, instrument[order->insidx]->m_pExchange);
}

void TraderYida::notifyOrder(const YDOrder *rtn, const YDInstrument *instrument, const YDAccount *account)
{
	if (rtn->OrderRef <= orderref_init)
		return;

	auto it = ref2id.find(rtn->OrderRef);

	if (it == ref2id.end())
		return;

	int orderid = it->second;

	if (rtn->OrderStatus == YD_OS_Queuing)
		trader_on_send_rtn(&trader, orderid, rtn->OrderSysID);
	else if (rtn->OrderStatus == YD_OS_Canceled)
		trader_on_cancel_rtn(&trader, orderid, rtn->OrderSysID);
	else if (rtn->OrderStatus == YD_OS_Rejected)
		trader_on_send_err(&trader, orderid, rtn->ErrorNo);
}

void TraderYida::notifyTrade(const YDTrade *rtn, const YDInstrument *pInstrument, const YDAccount *pAccount)
{
	if (rtn->OrderRef <= orderref_init)
		return;

	auto it = ref2id.find(rtn->OrderRef);

	if (it == ref2id.end())
		return;

	int orderid = it->second;
	struct trade trade;

	trade.orderid = orderid;
	trade.insidx = ins2idx(trader.instab, pInstrument->InstrumentID);
	trade.price = rtn->Price;
	trade.volume = rtn->Volume;
	trade.sysid = rtn->OrderSysID;

	trader_on_trade_rtn(&trader, &trade);
}

void TraderYida::notifyFailedOrder(const YDInputOrder *rtn, const YDInstrument *pInstrument, const YDAccount *pAccount)
{
	if (rtn->OrderRef <= orderref_init)
		return;

	auto it = ref2id.find(rtn->OrderRef);

	if (it == ref2id.end())
		return;

	int orderid = it->second;

	trader_on_send_err(&trader, orderid, rtn->ErrorNo);
}

void TraderYida::notifyFailedCancelOrder(const YDFailedCancelOrder *rtn,const YDExchange *pExchange, const YDAccount *pAccount)
{
	// trader_on_cancel_err(&trader, orderid, rtn->ErrorNo);
}


std::string Trim(const std::string & str1, const string& token = " \t\n\r", int type = 0)
{
	std::string str = str1;
	if (type == 0) {
		std::string::size_type pos = str.find_last_not_of(token);
		if (pos != std::string::npos) {
			str.erase(pos + 1);
			pos = str.find_first_not_of(token);
			if (pos != std::string::npos) str.erase(0, pos);
		}
		else str.erase(str.begin(), str.end());
	}
	else if (type == 1) {
		std::string::size_type pos = str.find_first_not_of(token);
		if (pos != std::string::npos) str.erase(0, pos);
		else str.erase(str.begin(), str.end());
	}
	else if (type == 2) {
		std::string::size_type pos = str.find_last_not_of(token);
		if (pos != std::string::npos) {
			str.erase(pos + 1);
		}
		else str.erase(str.begin(), str.end());
	}
	return str;
}

bool DelimitString(const std::string& str, std::list<std::string>& lst)
{
	size_t i, len;
	std::string temp = str, t;
	char lastChar;

	Trim(temp);
	const char* ptr = temp.c_str();
	len = temp.length();
	if (len < 1)
		return false;

	lst.clear();
	lastChar = ' ';
	i = 0;
	while (i < len)
	{
		if (ptr[i] == ' ' || ptr[i] == '\t' || ptr[i] == '\r' || ptr[i] == '\n') //为空格
		{
			if (lastChar != ' ')//一个段结束
			{
				t = temp.substr(0, i);
				lst.push_back(t);
				temp = temp.substr(i, len - i);
				len = len - i;
				i = 0;
				ptr = temp.c_str();
				lastChar = ' ';
			}
			else
			{
				i = i + 1;
			}
		}
		else
		{
			if (lastChar == ' ') //一个段开始
			{
				lastChar = ptr[i];
				temp = temp.substr(i, len - i);
				ptr = temp.c_str();
				len = len - i;
				i = 0;
			}
			else
			{
				i = i + 1;
			}
		} //end if
	} // end while
	if (len > 0)
		lst.push_back(temp);
	return true;
}

int DoCmd(const list<string>& cmdlst, list<string>::const_iterator it) {
	if (*it == "login")
	{
		yida->login(YIDA_CONFIG.c_str());
		return 0;
	}

	else if (*it == "logout")
	{
		yida->logout();
		return 0;
	}

	else if (*it == "order") //下单
	{
		//先设置默认值
		YDInputOrder new_order;
		new_order
		yida->new_order.Direction
		new_order.Direction = encode_direction(order->flags);
	new_order.OffsetFlag = encode_offset(order->flags);
	new_order.HedgeFlag = encode_hedge(order->flags);
	new_order.Price = order->price;
	new_order.OrderVolume = order->volume;
	new_order.OrderRef = ++orderref;
	new_order.OrderType = encode_timecond(order->flags);

	if (order->branchid != -1) {
		new_order.ConnectionSelectionType = YD_CS_Fixed;
		new_order.ConnectionID = order->branchid;
	} else {
		new_order.ConnectionSelectionType = YD_CS_Any;
	}



		SOrderParam param;
		param.hedgeType = _CHF_Speculation;
		param.offset = _OF_CloseToday;
		param.direction = _DT_Null;
		param.orderType = _OT_Normal;
		param.volume = 1;
		param.price = 0;
		param.exchangeId = "CFFEX";
		param.instrumentId = "";
		it++;
		while (it != cmdlst.end()) {
			if (*it == "/c") { //合约
				it++;
				param.instrumentId = it->c_str();
			}
			else if (*it == "/p") { //价格
				it++;
				param.price = atof(it->c_str());
			}
			else if (*it == "/v") { //数量
				it++;
				param.volume = atoi(it->c_str());
			}
			else if (*it == "buy") { //买
				param.direction = _DT_Buy;
			}
			else if (*it == "sell") { //卖
				param.direction = _DT_Sell;
			}
			else if (*it == "open") { //开仓
				param.offset = _OF_Open;
			}
			else if (*it == "close") { //平仓
				param.offset = _OF_Close;
			}
			else if (*it == "closetoday") { //平今
				param.offset = _OF_CloseToday;
			}
			else if (*it == "closeyes") { //平老
				param.offset = _OF_CloseYesterday;
			}
			else if (*it == "hedge") { //套保
				param.hedgeType = _CHF_Hedge;
			}
			else if (*it == "spec") { //投机
				param.hedgeType = _CHF_Speculation;
			}
			else if (*it == "arbi") { //套利
				param.hedgeType = _CHF_Arbitrage;
			}
			else if (*it == "marketmaker") {//做市商
				param.hedgeType = _CHF_MarketMaker;
			}
			else if (0 == strcmp(it->c_str(), "fak")) {
				param.orderType = _OT_FAK;
			}
			else if (0 == strcmp(it->c_str(), "fok")) {
				param.orderType = _OT_FOK;
			}
			it++;
		}
		int iResult = tsh->PutOrder(&param);
		return iResult != 0;
	}
	else if (*it == "cancel")//撤单
	{
		it++;
		char ordersysId[13] = { 0 };
		const char* instrumentId = "";
		const char* exchangeId = "";
		while (it != cmdlst.end()) {

			if (*it == "/n") { //报单编号
				it++;
				sprintf(ordersysId, "%12s", it->c_str());
			}
			if (*it == "/c") { //合约
				it++;
				instrumentId = it->c_str();
			}
			else if (0 == strcmp(it->c_str(), "cffex")) {
				exchangeId = "CFFEX";
			}
			else if (0 == strcmp(it->c_str(), "dce")) {
				exchangeId = "DCE";
			}
			else if (0 == strcmp(it->c_str(), "czce")) {
				exchangeId = "CZCE";
			}
			else if (0 == strcmp(it->c_str(), "shfe")) {
				exchangeId = "SHFE";
			}
			else if (0 == strcmp(it->c_str(), "ine")) {
				exchangeId = "INE";
			}
			else if (0 == strcmp(it->c_str(), "SSE")) {
				exchangeId = "SSE";
			}
			it++;
		}
		int re = tsh->CancelOrder(ordersysId, instrumentId, exchangeId);
		return re != 0;
	}

	else if (*it == "qryinst") //查询合约
	{
		const char* exch = NULL;
		const char* proid = NULL;
		const char* instr = NULL;
		it++;
		while (it != cmdlst.end()) {
			if (0 == strcmp(it->c_str(), "sse")) {
				exch = "SSE";
				it++;
			}
			if (0 == strcmp(it->c_str(), "szse")) {
				exch = "SZSE";
				it++;
			}
			else if (0 == strcmp(it->c_str(), "/c")) {
				it++;
				instr = it->c_str();
				it++;
			}
			else if (0 == strcmp(it->c_str(), "/p")) {
				it++;
				proid = it->c_str();
				it++;
			}
		}
		int re = tsh->QryInstrument(exch, proid, instr);
		return re != 0;
	}
	else if (*it == "qryorder") //查询委托
	{
		int iResult = tsh->QryOrder();
		return iResult != 0;
	}
	else if (*it == "qrytrade") //查询成交
	{
		int iResult = tsh->QryTrade();
		return iResult != 0;
	}
	else if (*it == "qryfund") //查询成交
	{
		int iResult = tsh->QryFund();
		return iResult != 0;
	}

	else if (*it == "exit")
		return -2;
	else
	{
		cout << "以下命令可用：\n";
		for (size_t i = 0; i < sizeof(g_helpInfo) / sizeof(char*); i += 2) {
			cout << g_helpInfo[i] << "\n\t";
			cout << g_helpInfo[i + 1] << "\n";
		}
	}
	return 0;


}

int DoCmd(const string& cmd) {

	list<string> cmdlst;
	if (!DelimitString(cmd, cmdlst) || cmdlst.size() < 1) {
		cerr << "paser cmd error\n";
		return -1;
	}
	return DoCmd(cmdlst, cmdlst.cbegin());
}

void getConfig()
{
	ifstream infile;
	const char* t1;
	char line[1024] = { 0 };
	infile.open("config.txt");
	char buffer[100];
	//getcwd(buffer, 100);
	cout << "read config.txt" << endl;
	while (infile.getline(line, sizeof(line)))
	{
		list<string> linelst;
		cout << line << endl;
		if (!DelimitString(line, linelst) || linelst.size() < 2)
		{
			cout << "info" << linelst.size() << " " << linelst.front() << " " << linelst.back() << endl;
			cerr << "config.txt has error" << endl;
		}
		list<string>::iterator itor = linelst.begin();
		list<string>::iterator itor2 = ++itor;
		if (linelst.front() == ("yida_config")) {
			YIDA_CONFIG = *itor2;
		}
		else if (linelst.front() == ("userid")) {
			USERID = *itor2;
		}
		else if (linelst.front() == ("pwd")) {
			PASSWORD = *itor2;
		}
		else if (linelst.front() == ("appid")) {
			APPID = *itor2;
		}
		else if (linelst.front() == ("authcode")) {
			AUTHCODE = *itor2;
		}
	}
	cout << USERID << " " << PASSWORD << " " << APPID << " " << AUTHCODE << " " << YIDA_CONFIG << endl;
}

int main(int argc, char *argv[]) {
	getConfig();
	yida = new TraderYida();
	char buf[1024];
	string cmd;
	while (true)
	{
		fgets(buf, sizeof(buf), stdin);
		cmd = (buf);

		cmd = Trim(cmd);
		if (cmd.empty()) {
			continue;
		}

		if (-2 == DoCmd(cmd))
			break;

	}
	return  0;
}
