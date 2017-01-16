/*
 * bankAccount.h
 *
 *  Created on: 30 ���� 2016
 *      Author: Eitan Levin
 */

#ifndef BANKACCOUNT_H_
#define BANKACCOUNT_H_

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define ERROR_VALUE (-1)
#define SUCCESS_VALUE (0)

#define ACCOUNT_NOT_ENOUGH_MONEY 0
#define ACCOUNT_SUCCESS 1
#define ACCOUNT_FROZEN 2
#define ACCOUNT_BALANCE_OVERFLOW 3

/*
 * Here will be all the simplest of functions.
 * This Data Structure assumes trust between it and the user.
 * Only parameters validation is required.
 */

class bankAccount {
private:
	int _id;
	int _password;
	int _balance;
	bool _isFrozen;
	int readBalanceCounter;
	int readFreezeCounter;
	pthread_mutex_t read_balance_lock;
	pthread_mutex_t write_balance_lock;
	pthread_mutex_t read_freeze_lock;
	pthread_mutex_t write_freeze_lock;
public:
	//Empty Constructor:
	bankAccount() : _id(0), _password(0), _balance (0),
		_isFrozen(false), readBalanceCounter(0), readFreezeCounter(0) {
		pthread_mutex_init(&read_balance_lock, NULL);
		pthread_mutex_init(&write_balance_lock, NULL);
		pthread_mutex_init(&read_freeze_lock, NULL);
		pthread_mutex_init(&write_freeze_lock, NULL);
	}
	//Constructor:
	bankAccount(int accountNumber, int accountPass, int balance, bool isFrozen = false) :
		_id(accountNumber), _password(accountPass), _balance (balance), _isFrozen(isFrozen),
		readBalanceCounter(0), readFreezeCounter(0) {
		pthread_mutex_init(&read_balance_lock, NULL);
		pthread_mutex_init(&write_balance_lock, NULL);
		pthread_mutex_init(&read_freeze_lock, NULL);
		pthread_mutex_init(&write_freeze_lock, NULL);
	}
	int getNumber();
	int getPassword();
	int getBalance();
	int getBalanceNoSleep();
	bool isAccountFrozen();//True if frozen, false if not frozen
	bool freeze();//False if already frozen, true otherwise
	bool unFreeze();//False if already not frozen, true otherwise
	int withrawMoney(int withrawSum, int* currentBalance);
	int withrawMoneyForCommission(int percentage);//Return commission sum
	int depositMoney(int depositSum, int* currentBalance);
	void printAccount();
	void lockAccount();//Use only for money transfer!
	void unLockAccount();//Use only for money transfer!
	bool transferWithdraw(int withrawSum);//Use only for money transfer and only after lockAccount!
	bool transferDeposit(int depositSum);//Use only for money transfer and only after lockAccount!
	int transferCheckBalance();//Use only for money transfer and only after lockAccount!
	bool transferIsFrozen();//Use only for money transfer and only after lockAccount!
	bool freezeReadStatusAndMarkReaders();
	void freezeStatusUnMarkReaders();
	bool lockForTransfer();//Use only for money transfer! Return false if frozen.
	void unLockForTransfer();//Use only for money transfer!
	~bankAccount();
};



#endif /* BANKACCOUNT_H_ */
