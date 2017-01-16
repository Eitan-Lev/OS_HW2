/*
 * bankMap.h
 *
 *
 *      Author: Eitan Levin
 */

#ifndef BANK_MAP_H_
#define BANK_MAP_H_

#include <map>
#include <stdlib.h>
#include "bankAccount.h"
#include "Exceptions.h"

#define TRANSFER_SUCCESSFUL 0
#define FIRST_ACCOUNT_FROZEN 1
#define SECOND_ACCOUNT_FROZEN 2
#define SAME_ACCOUNT 3



/*
 * In this file all the top level functions will be.
 * Any validation of password or otherwise is done here.
 * All actions are implemented here.
 */

using std::map;
typedef map<int, bankAccount> AccountsMap;
typedef AccountsMap::value_type Pair;
typedef AccountsMap::iterator Iterator;
typedef AccountsMap::const_iterator IteratorConst;

class bankMap {
private:
	AccountsMap _innerMap;
	IteratorConst accountIter;
public:

	void openNewAccount(int accountNumber, int accountPass, int balance);
	int getAccountBalance(int accountNumber, int accountPass);
	void freezeAccount(int accountNumber, int accountPass);
	void unFreezeAccount(int accountNumber, int accountPass);
	int depositToAccount(int accountNumber, int accountPass, int depositSum);//return balance
	int withrawFromAccount(int accountNumber, int accountPass, int withrawSum);//return balance
	int transferMoneyAndSaveBalances(int srcAccountNumber, int srcAccountPass,
			int destAccountNumber, int amount, int* srcBalance, int* dstBalance, int* frozenAccount);
	int takeComission(int accountNumber, int percentage);
	int getPassword(int accountNumber);
	bool checkPassword(int accountNumber, int accountPass);
	bool isAccountInMap(int accountNumber);
	void printAccountInMap(int accountNumber);
	IteratorConst begin();
	IteratorConst end();
};

#endif /* BANK_MAP_H_ */
