/*
 * Exceptions.h
 *
 *  Created on: 30 ���� 2016
 *      Author: Eitan Levin
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

class OSException {};

class BankAccountException : public OSException {};

class AccountNumberAlreadyExistsException : public BankAccountException {};//Can happen when creating a new account
class AccountDoesntExistException : public BankAccountException {};//Should never happen
class InvalidPasswordException : public BankAccountException {};//Should never happen
class WrongPasswordException : public BankAccountException {};
class NotEnoughMoneyException : public BankAccountException {};
class BalanceOverflowException : public BankAccountException {};//Should never happen
class AccountIsFrozenException : public BankAccountException {};





#endif /* EXCEPTIONS_H_ */
