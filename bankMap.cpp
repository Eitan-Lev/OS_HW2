/*
 * bankMap.c
 *
 *      Author: Eitan Levin
 */
#include "bankMap.h"
#include <ctime>
#include <cstdlib>

#include <unistd.h>

/*
 * ****openNewAccount****
 * Open a new account.
 * Parameters: the account number, the account password, the account balance.
 * Error Values:
 * 		AccountNumberAlreadyExistsException if account already exists in map.
 * 		InvalidPasswordException if password is not 4 digits.
 * Print Requirements: On account already exists, on account open success.
 */
void bankMap::openNewAccount(int accountNumber, int accountPass, int balance) {
	if (this->isAccountInMap(accountNumber) == true) {
		throw AccountNumberAlreadyExistsException();
	}
	bankAccount newAccount(accountNumber, accountPass, balance);
	Pair newPair(accountNumber, newAccount);
	this->_innerMap.insert(newPair);
}

/*
 * ****getAccountBalance****
 * Return the account balance.
 * Parameters: the account number, the account password.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * Return Values: the account balance.
 * Print Requirements: On wrong password, on account doesn't exist, on success.
 */
int bankMap::getAccountBalance(int accountNumber, int accountPass) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	}
	if(this->checkPassword(accountNumber, accountPass == false)) {
		throw WrongPasswordException();
	}
	return this->_innerMap[accountNumber].getBalance();
}

/*
 * ****freezeAccount****
 * Freeze a given account.
 * Parameters: the account number, the account password.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * Print Requirements: On wrong password, on account doesn't exist.
 */
void bankMap::freezeAccount(int accountNumber, int accountPass) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	}
	if(this->checkPassword(accountNumber, accountPass) == false) {
		throw WrongPasswordException();
	}
	this->_innerMap[accountNumber].freeze();
}

/*
 * ****unFreezeAccount****
 * UnFreeze a given account.
 * Parameters: the account number, the account password.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * Print Requirements: On wrong password, on account doesn't exist.
 */
void bankMap::unFreezeAccount(int accountNumber, int accountPass) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	}
	if(this->checkPassword(accountNumber, accountPass) == false) {
		throw WrongPasswordException();
	}
	this->_innerMap[accountNumber].unFreeze();
}

/*
 * ****depositToAccont****
 * Deposit sum of money to a given account.
 * Parameters: the account number, the account password, deposit sum.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * 		BalanceOverflowException if deposit causes int balance overflow.
 * Print Requirements: On wrong password, on account doesn't exist, on successful deposit.
 */
int bankMap::depositToAccount(int accountNumber, int accountPass, int depositSum) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	} else if(this->checkPassword(accountNumber, accountPass) == false) {
		throw WrongPasswordException();
	}
	int currentBalance = ERROR_VALUE;//Illegal value, for debugging
	int depositResult = this->_innerMap[accountNumber].depositMoney(depositSum, &currentBalance);
	if (depositResult == ACCOUNT_FROZEN) {//Account is frozen
		throw AccountIsFrozenException();
	} else if (depositResult == ACCOUNT_BALANCE_OVERFLOW) {//Balance overflow
		throw BalanceOverflowException();
	}
	return currentBalance;
}

/*
 * ****withrawFromAccont****
 * Withdraw sum of money from a given account.
 * Parameters: the account number, the account password, withdraw sum.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * 		NotEnoughMoneyException if balance not enough for withdrawal.
 * Print Requirements: On wrong password, on account doesn't exist, on not enough balance, on successful deposit.
 */
int bankMap::withrawFromAccount(int accountNumber, int accountPass, int withrawSum) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	} else if(this->checkPassword(accountNumber, accountPass) == false) {
		throw WrongPasswordException();
	}
	int currentBalance = ERROR_VALUE;//Illegal value, for debugging
	int resWithdraw = this->_innerMap[accountNumber].withrawMoney(withrawSum, &currentBalance);
	if(resWithdraw == ACCOUNT_NOT_ENOUGH_MONEY) {//Not enough money
		throw NotEnoughMoneyException();
	} else if(resWithdraw == ACCOUNT_FROZEN) {//Frozen
		throw AccountIsFrozenException();
	}
	return currentBalance;
}

/*
 * ****transferMoneyAndSaveBalances****
 * Transfer money between given accounts.
 * Parameters: 	source account number, source account password, destination account number, amount to transfer,
 * 				source account balance and destination account balance (to return),
 * 				which of the accounts is frozen, if any (to return).
 * Error Values:
 * 		AccountDoesntExistException if either account doesn't exist in map.
 * 		WrongPasswordException if password is not the correct one.
 * 		NotEnoughMoneyException if balance not enough for transfer.
 * Print Requirements: On wrong password, on account doesn't exist, on not enough balance, on successful deposit.
 */
int bankMap::transferMoneyAndSaveBalances(int srcAccountNumber, int srcAccountPass,
		int dstAccountNumber, int amount, int* srcBalance, int* dstBalance, int* frozenAccount) {
	if (this->isAccountInMap(srcAccountNumber) == false || this->isAccountInMap(dstAccountNumber) == false) {
		throw AccountDoesntExistException();
	} else if (this->checkPassword(srcAccountNumber, srcAccountPass) == false) {
		throw WrongPasswordException();
	}
	int firstAccount = (srcAccountNumber >= dstAccountNumber) ? dstAccountNumber : srcAccountNumber;//To prevent deadlocks
	int secondAccount = (srcAccountNumber >= dstAccountNumber) ? srcAccountNumber : dstAccountNumber;//To prevent deadlocks
	bool isNotFrozen = this->_innerMap[firstAccount].lockForTransfer();
	if(isNotFrozen == false) {
		*frozenAccount = firstAccount;
		this->_innerMap[firstAccount].unLockForTransfer();
		throw AccountIsFrozenException();
	}
	if (srcAccountNumber == dstAccountNumber) {//Definitely not frozen
		*srcBalance = this->_innerMap[firstAccount].transferCheckBalance();
		*dstBalance = *srcBalance;
		this->_innerMap[firstAccount].unLockForTransfer();
		return SAME_ACCOUNT;//Same account. Legal, but acknowledge this
	}
	isNotFrozen = this->_innerMap[secondAccount].lockForTransfer();
	sleep(1);
	if(isNotFrozen == false) {
		this->_innerMap[firstAccount].unLockForTransfer();
		this->_innerMap[secondAccount].unLockForTransfer();
		*frozenAccount = secondAccount;
		throw AccountIsFrozenException();
	}
	bool withdrawRes = this->_innerMap[srcAccountNumber].transferWithdraw(amount);
	if (withdrawRes == true) {
		this->_innerMap[dstAccountNumber].transferDeposit(amount);
	}
	*srcBalance = this->_innerMap[srcAccountNumber].transferCheckBalance();
	*dstBalance = this->_innerMap[dstAccountNumber].transferCheckBalance();
	this->_innerMap[secondAccount].unLockForTransfer();
	this->_innerMap[firstAccount].unLockForTransfer();
	if (withdrawRes == false) {
		throw NotEnoughMoneyException();
	}
	return TRANSFER_SUCCESSFUL;
}

/*
 * ****takeComission****
 * Take commission from given account.
 * Parameters: the account number.
 * Error Values:
 * 		AccountDoesntExistException if account doesn't exist in map.
 * Print Requirements: if account dosen't exist.
 * Return Values: the commission taken.
 */
int bankMap::takeComission(int accountNumber, int percentage) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	}
	int commission = this->_innerMap[accountNumber].withrawMoneyForCommission(percentage);
	return commission;
}

/*
 * ****getPassword****
 * Get a given account password.
 * Parameters: the account number.
 * Error Values:
 * 		AccountDoesntExistException if the account doesn't exist in map.
 * Return Values: account password.
 */
int bankMap::getPassword(int accountNumber) {
	if (this->isAccountInMap(accountNumber) == false) {
		throw AccountDoesntExistException();
	}
	return this->_innerMap[accountNumber].getPassword();
}

/*
 * ****checkPassword****
 * Check if the account with "accountNumber" has the password "accountPass"
 * Parameters: the account number, the password.
 * Return Values: true- if password matches.
 *				  false- otherwise.
 */
bool bankMap::checkPassword(int accountNumber, int accountPass) {
	int realPass = this->getPassword(accountNumber);
	return (realPass == accountPass) ? true : false;
}

/*
 * ****isAccountInMap****
 * Check if given account exists in map.
 * Parameters: the account number.
 * Return Values: true if exists, false if doesn't.
 */
bool bankMap::isAccountInMap(int accountNumber) {
	if(this->_innerMap.find(accountNumber) == this->_innerMap.end()) {
		return false;
	}
	return true;
}

IteratorConst bankMap::begin() {
	return (this->_innerMap).begin();
}

IteratorConst bankMap::end() {
	return (this->_innerMap).end();
}

void bankMap::printAccountInMap(int accountNumber) {
	this->_innerMap[accountNumber].printAccount();
}

