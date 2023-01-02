#include "ydApi.h"

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

using namespace std;

class TraderYida : public YDListener {
public:
	TraderYida(cfg_t *cfg, struct memdb *memdb);
	int login(const char *configFilename);
	int logout();
	inline int send_order(struct order *order);
	inline int cancel_order(struct order *order);
	inline void load_config(cfg_t *cfg);
	struct trader trader;
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

	cfg_t *cfg;
	YDApi *api;
	struct TraderYidaAccount {
		const char *config;
		const char *userid;
		const char *passwd;
		const char *authcode;
		const char *appid;
	} acc;

	YDInputOrder new_order;
	YDCancelOrder cancel;

	map<int, int> id2ref;
	map<int, int> ref2id;

	int orderref_init;
	int orderref;
	const YDInstrument *instrument[MAX_INSTRUMENT_NR];
	volatile int login_finished;
	volatile int logout_finished;
};

TraderYida::TraderYida(cfg_t *cfg, struct memdb *memdb) : cfg(cfg)
{
	trader_init(&trader, cfg, memdb);

	memset(&new_order, 0, sizeof(new_order));
	memset(&cancel, 0, sizeof(cancel));

	load_config(cfg);
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
	wflog_msg("try logout");
	api->startDestroy();

	while (!logout_finished)
		usleep(1000);

	return 0;
}

void TraderYida::notifyReadyForLogin(bool hasLoginFailed)
{
	if (!cfg_get_string(cfg, "userid", &acc.userid)
	    || !cfg_get_string(cfg, "password", &acc.passwd)
	    || !cfg_get_string(cfg, "authcode", &acc.authcode)
	    || !cfg_get_string(cfg, "appid", &acc.appid))
		wflog_exit(-1, "cfg read `userid', `password', `authcode' or `appid' error");

	wflog_msg("try login %s", acc.userid);

	if (!api->login(acc.userid, acc.passwd, acc.appid, acc.authcode))
		wflog_exit(-1, "cannot login: hasLoginFailed = %d, errno = %d", hasLoginFailed, errno);
}

void TraderYida::notifyLogin(int errorNo, int maxOrderRef, bool isMonitor)
{
	if (errorNo == 0) {
		orderref = orderref_init = maxOrderRef;
		wflog_msg("login successfully %d", orderref);
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
	wflog_msg("logout successfully");
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

inline void TraderYida::load_config(cfg_t *cfg)
{
	return;

	int branchid;

	if (!cfg_get_int(cfg, "branchid", &branchid))
		wflog_exit(-1, "cfg read `branchid' error");

	trader_set_branchid(&trader, branchid);

	wflog_msg("load branchid = %d", branchid);
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

int main(int argc, char *argv[]) {

}
