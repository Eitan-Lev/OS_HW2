/*
 * bankAccount.cpp
 *
 *  Created on: 30 ���� 2016
 *      Author: Eitan Levin
 */
#include "bankAccount.h"
#include <iostream>
#include <assert.h>
#include "MacroBank.h"

using std::cout;
using std::endl;

int bankAccount::getNumber() {//No need for locks, once account is created, number never changes.
	return this->_id;
}

int bankAccount::getPassword() {//No need for locks, once account is created, password never changes.
	return this->_password;
}

int bankAccount::getBalance() {
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter++;//Indicate 1 more thread is reading
	if (readBalanceCounter == 1) {//First reader
		pthread_mutex_lock(&write_balance_lock);
	}
	sleep(1);
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	int currentBalance = _balance;//No need to be locked, all readers can pull together. Writers are locked.
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter--;//Indicate 1 less thread is reading
	if (readBalanceCounter == 0) {//Last to read
		pthread_mutex_unlock(&write_balance_lock);
	}
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	return currentBalance;
}

int bankAccount::getBalanceNoSleep() {
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter++;//Indicate 1 more thread is reading
	if (readBalanceCounter == 1) {//First reader
		pthread_mutex_lock(&write_balance_lock);
	}
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	int currentBalance = _balance;//No need to be locked, all readers can pull together. Writers are locked.
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter--;//Indicate 1 less thread is reading
	if (readBalanceCounter == 0) {//Last to read
		pthread_mutex_unlock(&write_balance_lock);
	}
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	return currentBalance;
}

bool bankAccount::isAccountFrozen() {
	pthread_mutex_lock(&read_freeze_lock);//Lock readers
	readFreezeCounter++;//Indicate 1 more thread is reading
	if (readFreezeCounter == 1) {//First reader
		pthread_mutex_lock(&write_freeze_lock);
	}
	pthread_mutex_unlock(&read_freeze_lock);//Unlock readers
	bool currentFreezeStatus = _isFrozen;//No need to be locked, all readers can pull together. Writers are locked.
	pthread_mutex_lock(&read_freeze_lock);//Lock readers
	readFreezeCounter--;//Indicate 1 less thread is reading
	if (readFreezeCounter == 0) {//Last to read
		pthread_mutex_unlock(&write_freeze_lock);
	}
	pthread_mutex_unlock(&read_freeze_lock);//Unlock readers
	return currentFreezeStatus;
}

bool bankAccount::freeze() {
	bool resFreeze;
	pthread_mutex_lock(&write_freeze_lock);
	if (this->_isFrozen == true) {
		resFreeze = false;
	} else {
		this->_isFrozen = true;
	}
	pthread_mutex_unlock(&write_freeze_lock);
	return resFreeze;
}
bool bankAccount::unFreeze() {
	bool resFreeze;
	pthread_mutex_lock(&write_freeze_lock);
	if (this->_isFrozen == false) {
		resFreeze = false;
	} else {
		this->_isFrozen = false;
	}
	pthread_mutex_unlock(&write_freeze_lock);
	return resFreeze;
}

int bankAccount::withrawMoney(int withrawSum, int* currentBalance) {
	int tmpBalance, resVar;
	bool isFrozen = this->freezeReadStatusAndMarkReaders();
	if (isFrozen == true) {
		resVar = ACCOUNT_FROZEN;
	} else {
		pthread_mutex_lock(&write_balance_lock);
		sleep(1);
		tmpBalance = this->_balance;
		if (withrawSum > tmpBalance) {
			resVar = ACCOUNT_NOT_ENOUGH_MONEY;
		} else {
			this->_balance -= withrawSum;
			*currentBalance = this->_balance;
			resVar = ACCOUNT_SUCCESS;
		}
		pthread_mutex_unlock(&write_balance_lock);
	}
	this->freezeStatusUnMarkReaders();
	return resVar;
}

int bankAccount::withrawMoneyForCommission(int percentage) {
	int commission;
	pthread_mutex_lock(&write_balance_lock);
	commission = (this->_balance * percentage)/100;
	if (commission > this->_balance) {//Should never happen. For debugging.
		exit(ERROR_VALUE);
	}
	this->_balance -= commission;
	pthread_mutex_unlock(&write_balance_lock);
	return commission;
}

int bankAccount::depositMoney(int depositSum, int* currentBalance) {
	int tmpBalance, resVar;
	bool isFrozen = this->freezeReadStatusAndMarkReaders();
	if (isFrozen == true) {
		resVar = ACCOUNT_FROZEN;
	} else {
		pthread_mutex_lock(&write_balance_lock);
		sleep(1);
		tmpBalance = this->_balance;
		if ((depositSum + tmpBalance) < tmpBalance) {//int overflow, Should never happen
			resVar = ACCOUNT_BALANCE_OVERFLOW;//Overflow Failure
		} else {
			*currentBalance = this->_balance;
			this->_balance += depositSum;
			resVar = ACCOUNT_SUCCESS;
		}
		pthread_mutex_unlock(&write_balance_lock);
	}
	this->freezeStatusUnMarkReaders();
	return resVar;
}

void bankAccount::printAccount() {
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter++;//Indicate 1 more thread is reading
	if (readBalanceCounter == 1) {//First reader
		pthread_mutex_lock(&write_balance_lock);
	}
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	int currentBalance = this->_balance;//No need to be locked, all readers can pull together. Writers are locked.
	pthread_mutex_lock(&read_balance_lock);//Lock readers
	readBalanceCounter--;//Indicate 1 less thread is reading
	if (readBalanceCounter == 0) {//Last to read
		pthread_mutex_unlock(&write_balance_lock);
	}
	pthread_mutex_unlock(&read_balance_lock);//Unlock readers
	cout << "Account " << this->_id << ": Balance - " << currentBalance <<
			"$ , Account Password - " << this->_password << endl;
}

bool bankAccount::lockForTransfer() {//Use only for money transfer!
	bool freezeStatus = this->freezeReadStatusAndMarkReaders();//Keep account frozen while changing balance.
	if (freezeStatus == true) {
		this->freezeStatusUnMarkReaders();//If frozen, don't perform transfer.
		return false;
	}
	pthread_mutex_lock(&write_balance_lock);//Once account is confirmed to be unfrozen, lock for changing balance.
	return true;
}

void bankAccount::unLockForTransfer() {//Use only for money transfer!
	pthread_mutex_unlock(&write_balance_lock);//Now account can be changed
	this->freezeStatusUnMarkReaders();//It is now ok to freeze account.
}

bool bankAccount::transferWithdraw(int withrawSum) {
	if (withrawSum > (this->_balance)) {
		return false;
	} else {
		this->_balance -= withrawSum;
		return true;
	}
}

bool bankAccount::transferDeposit(int depositSum) {
	if ((depositSum + this->_balance) < (this->_balance)) {//int overflow, Should never happen, for debugging.
		std::cout << "Overflow" << std::endl;
		exit(ERROR_VALUE);
	} else {
		this->_balance += depositSum;
		return true;
	}
}

int bankAccount::transferCheckBalance() {
	return this->_balance;
}

bool bankAccount::transferIsFrozen() {
	return this->_isFrozen;
}

bool bankAccount::freezeReadStatusAndMarkReaders() {
	pthread_mutex_lock(&read_freeze_lock);//Lock readers
	readFreezeCounter++;//Indicate 1 more thread is reading
	if (readFreezeCounter == 1) {//First reader
		pthread_mutex_lock(&write_freeze_lock);
	}
	pthread_mutex_unlock(&read_freeze_lock);//Unlock readers
	return this->_isFrozen;//No need to be locked, all readers can pull together. Writers are locked.
}

void bankAccount::freezeStatusUnMarkReaders() {
	pthread_mutex_lock(&read_freeze_lock);//Lock readers
	readFreezeCounter--;//Indicate 1 less thread is reading
	if (readFreezeCounter == 0) {//Last to read
		pthread_mutex_unlock(&write_freeze_lock);
	}
	pthread_mutex_unlock(&read_freeze_lock);//Unlock readers
}

bankAccount::~bankAccount() {
	pthread_mutex_destroy(&read_balance_lock);
	pthread_mutex_destroy(&write_balance_lock);
	pthread_mutex_destroy(&read_freeze_lock);
	pthread_mutex_destroy(&write_freeze_lock);
}
