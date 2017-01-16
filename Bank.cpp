/*
 * Bank.cpp
 *
 *  Created on: Dec 31, 2016
 *      Author: eitanlev
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "bankMap.h"
#include "bankAccount.h"
#include "Exceptions.h"
#include "MacroBank.h"

using std::stringstream;
using std::fstream;
using std::string;
using std::cout;
using std::endl;
using std::getline;
using std::ofstream;

/*
 * In this file the threads will be managed- Init thread, ATM thread, Commission thread and Bank Status thread.
 * All functions called from here must be sent with the accounts map.
 */

//Global map of accounts:
bankMap bankAccountsMap;


class ATM {//Each ATM thread will get this
public:
	int _id;
	char* commandsList;
};

//Locks, Variables and Functions used by the threads:
pthread_mutex_t map_lock;
pthread_mutex_t bank_balance_lock;
pthread_mutex_t log_file_lock;
ofstream log_file;
void* Continuous_Print_Run(void* cmds);
void* Commission_Run(void* cmds);
void* ATM_Run(void* cmds);
int BankBalance;
bool atmThreadRunning;
bool commissionThreadRunning;

//This is the map for the bank accounts:


int main(int argc, char* argv[]) {
	//================================================================
	//First check all parameters are OK:
	if (argc <= 2) { //Check at least 3 arguments- File, Number of ATMs, Input File.
		fprintf(stderr, "illegal arguments\n");
		exit(ERROR_VALUE);
	}

	int ATMs_NUMBER = atoi(argv[1]);
	if(argc != ATMs_NUMBER + 2) {
		fprintf(stderr,"Given number of ATMs doesn't match the actual amount of ATMs as arguments\n");
		exit(ERROR_VALUE);
	}
	for(int i = 2; i < argc; ++i ){ //checking ATM files
		if ((stringstream(argv[2])).good() == false) {
			fprintf(stderr, "At least one of the files doesn't exist or isn't readable\n");
			exit(ERROR_VALUE);
		}
	}
	//================================================================
	//Initialize locks:
	pthread_mutex_init(&map_lock, NULL);
	pthread_mutex_init(&bank_balance_lock, NULL);
	pthread_mutex_init(&log_file_lock, NULL);
	//================================================================
	//Preparing all threads and data:
	atmThreadRunning = false;
	commissionThreadRunning = false;
	BankBalance = 0;
	log_file.open("log.txt");
 	pthread_t threads_ATM[ATMs_NUMBER];
	pthread_t thread_Comission;
	pthread_t thread_Continuous_Print;
	ATM* ATM_Manage = new ATM[ATMs_NUMBER];
	int cmdsCounter = 2;
	int atmCounter = 1;
	for (int i = 0; i < ATMs_NUMBER; i++) {
		ATM_Manage[i]._id = atmCounter++;
		ATM_Manage[i].commandsList = argv[cmdsCounter++];
	}
	//================================================================
	//Create threads for ATM, commission and printing status:
	int threadRes;
	for (int i = 0; i < ATMs_NUMBER; i++) {
		threadRes = pthread_create(&threads_ATM[i], NULL, ATM_Run, &ATM_Manage[i]);
		if (threadRes != SUCCESS_VALUE) {
			fprintf(stderr, "error: pthread_create, threadRes: %d\n", threadRes);
			exit (ERROR_VALUE);
		}
	}
	atmThreadRunning = true;
	threadRes = pthread_create(&thread_Comission, NULL, Commission_Run, NULL);
	if (threadRes != SUCCESS_VALUE) {
		fprintf(stderr, "error: pthread_create, threadRes: %d\n", threadRes);
		exit (ERROR_VALUE);
	}
	threadRes = pthread_create(&thread_Continuous_Print, NULL, Continuous_Print_Run, NULL);
	if (threadRes != SUCCESS_VALUE) {
		fprintf(stderr, "error: pthread_create, threadRes: %d\n", threadRes);
		exit (ERROR_VALUE);
	}
	//================================================================
	//Join all ATM's:
	for (int i = 0; i < ATMs_NUMBER; i++) {
		threadRes = pthread_join(threads_ATM[i], NULL);
		//For debugging and Error catching:
		if (threadRes != SUCCESS_VALUE) {
			printf("pthread_join failure:\n");
			printf("threads_ATM[%d]:\n", i);
			printf("threadRes = %d \n", threadRes);
			exit(ERROR_VALUE);
		}
	}
	atmThreadRunning = false;
	threadRes = pthread_join(thread_Comission, NULL);
	//For debugging and Error catching:
	if (threadRes != SUCCESS_VALUE) {
		printf("pthread_join failure:\n");
		printf("thread_Comission, ");
		printf("threadRes = %d \n", threadRes);
		exit(ERROR_VALUE);
	}
	threadRes = pthread_join(thread_Continuous_Print, NULL);
	//For debugging and Error catching:
	if (threadRes != SUCCESS_VALUE) {
		printf("pthread_join failure:\n");
		printf("thread_Continuous_Print, ");
		printf("threadRes = %d \n", threadRes);
		exit(ERROR_VALUE);
	}
	//================================================================
	//Destroy locks, free pointers and close file:
	log_file.close();
	pthread_mutex_destroy(&map_lock);
	pthread_mutex_destroy(&bank_balance_lock);
	pthread_mutex_destroy(&log_file_lock);
	delete[] ATM_Manage;
	return 0;
}

void* ATM_Run(void* cmds) {
	//================================================================
	//Initialize variables:
	ATM* atm = (ATM*)cmds;
    fstream cmdList(atm->commandsList);
	ASSERT_VALID(cmdList, "File not found");
	ASSERT_VALID(cmdList.good(), "File not found");
	string cmdLine;
	int accountNumber, password, atmNumber, balance, sum, dstAccountNumber;
	char opCode;
	bool isAccountCurrentlyInMap;
	//================================================================
	while (getline(cmdList, cmdLine)) {
		stringstream command(cmdLine);
		//================================================================
		//Define variables and initialize checks
		ASSERT_VALID(command >> opCode >> accountNumber >> password, "Error in line format");//Add all arguments to command stream.
		if (opCode == 'O' || opCode == 'D' || opCode == 'W') {
			ASSERT_VALID(command >> sum, "Error in line format");//Cmd is open, deposit, or withdraw so add sum to command stream.
		}
		atmNumber = atm->_id;
		pthread_mutex_lock(&map_lock);
		isAccountCurrentlyInMap = bankAccountsMap.isAccountInMap(accountNumber);//If account doesn't exist here, we consider it non existing
		pthread_mutex_unlock(&map_lock);
		if (opCode != 'O' && isAccountCurrentlyInMap == true) {//Then Open command is not expected
			if (!(bankAccountsMap.checkPassword(accountNumber, password))) {//If not Open command, password needs to be correct
				LOG_WRONG_PASSWORD(atmNumber, accountNumber);
				continue;
			}
		} else if (opCode != 'O' && isAccountCurrentlyInMap == false) {
			if (opCode == 'L' || opCode == 'U') {
				continue;
			}
			LOG_ACCOUNT_DOESNT_EXIST(atmNumber, accountNumber);
			continue;
		}
		//================================================================
		//Deal with each command:
		if (opCode == 'O') {//Open Account
			if (isAccountCurrentlyInMap == true) {
				LOG_ACCOUNT_ALREADY_EXISTS(atmNumber);
			} else {
				pthread_mutex_lock(&map_lock); // not allowing two ATMs to add the same account - atomic check&add
				sleep(1);
				try {
					bankAccountsMap.openNewAccount(accountNumber, password, sum);
					LOG_OPEN_NEW_ACCOUNT(atmNumber, accountNumber, password, sum);
					pthread_mutex_unlock(&map_lock);
				} catch (AccountNumberAlreadyExistsException&) {
					LOG_ACCOUNT_ALREADY_EXISTS(atmNumber);
					pthread_mutex_unlock(&map_lock);
				} catch (...) {
					pthread_mutex_unlock(&map_lock);
					UNEXPECTED_EXCEPTION();
				}
			}
		}  else if (opCode == 'T') {//Transfer money between accounts
			ASSERT_VALID(command >> dstAccountNumber >> sum, "Error in line format");
			pthread_mutex_lock(&map_lock);
			bool isDstCurrentlyInMap = bankAccountsMap.isAccountInMap(dstAccountNumber);
			pthread_mutex_unlock(&map_lock);
			int srcNewBalance = ERROR_VALUE, dstNewBalance = ERROR_VALUE, frozenAccount = ERROR_VALUE;
			if (isDstCurrentlyInMap == false) {
				LOG_ACCOUNT_DOESNT_EXIST(atmNumber, dstAccountNumber);
			} else {
				try	{
					int resTransfer = bankAccountsMap.transferMoneyAndSaveBalances(accountNumber, password,
							dstAccountNumber, sum, &srcNewBalance, &dstNewBalance, &frozenAccount);//Result used for debugging
					if (srcNewBalance == ERROR_VALUE || dstNewBalance == ERROR_VALUE) {
						UNEXPECTED_EXCEPTION();//Transfer should have been successful. For debugging, should never happen.
					} else {
						LOG_TRANSFER(atmNumber, sum, accountNumber, dstAccountNumber, srcNewBalance, dstNewBalance);
					}
				} catch(AccountIsFrozenException&) {
					if (frozenAccount == ERROR_VALUE) {//If one of the accounts is frozen but var wasn't changed, something went wrong.
						UNEXPECTED_EXCEPTION();//For debugging, should never happen.
					}
					LOG_WRONG_PASSWORD(atmNumber, frozenAccount);
				} catch (NotEnoughMoneyException&) {
					LOG_NOT_ENOUGH_MONEY(atmNumber, accountNumber, sum);
				} catch (WrongPasswordException&) {
					WRONG_PASSWORD_ILLEGALY();
				} catch (AccountDoesntExistException&) {
					ACCOUNT_DOESNT_EXIST_ILLEGALY();
				} catch (...) {
					UNEXPECTED_EXCEPTION();
				}
			}
		} else if (opCode == 'L') {//Freeze account
			try {
				bankAccountsMap.freezeAccount(accountNumber, password);
			} catch (WrongPasswordException&) {
				WRONG_PASSWORD_ILLEGALY();
			} catch (AccountDoesntExistException&) {
				ACCOUNT_DOESNT_EXIST_ILLEGALY();
			} catch (...) {
				UNEXPECTED_EXCEPTION();
			}
		} else if (opCode == 'U') {//UnFreeze account
			try {
				bankAccountsMap.unFreezeAccount(accountNumber, password);
			} catch (WrongPasswordException&) {
				WRONG_PASSWORD_ILLEGALY();
			} catch (AccountDoesntExistException&) {
				ACCOUNT_DOESNT_EXIST_ILLEGALY();
			} catch (...) {
				UNEXPECTED_EXCEPTION();
			}
		} else if (opCode == 'D') {//Deposit
			try	{
				balance = bankAccountsMap.depositToAccount(accountNumber, password, sum);
				if (balance == ERROR_VALUE) {//newBalance should have changed. For debugging, should never happen.
					UNEXPECTED_EXCEPTION();//If not thrown and newBalance isn't legal, something went wrong.
				} else {
					LOG_DEPOSIT(atmNumber, accountNumber, balance, sum);
				}
			} catch (WrongPasswordException&) {
				WRONG_PASSWORD_ILLEGALY();
			} catch(AccountIsFrozenException&) {
				LOG_WRONG_PASSWORD(atmNumber, accountNumber);//Frozen
			} catch (AccountDoesntExistException&) {
				ACCOUNT_DOESNT_EXIST_ILLEGALY();
			} catch (BalanceOverflowException&) {
				BALANCE_OVERFLOW_ILLEGALY();
			} catch (...) {
				UNEXPECTED_EXCEPTION();
			}
		} else if (opCode == 'W') {//Withdraw
			try	{
				balance = bankAccountsMap.withrawFromAccount(accountNumber, password, sum);
				if (balance == ERROR_VALUE) {//newBalance should have changed. For debugging, should never happen.
					UNEXPECTED_EXCEPTION();//If not thrown and newBalance isn't legal, something went wrong.
				} else {
					LOG_WITHDRAW(atmNumber, accountNumber, balance, sum);
				}
			} catch (WrongPasswordException&) {
				WRONG_PASSWORD_ILLEGALY();
			} catch(AccountIsFrozenException&) {
				LOG_WRONG_PASSWORD(atmNumber, accountNumber);//Frozen
			} catch (AccountDoesntExistException&) {
				ACCOUNT_DOESNT_EXIST_ILLEGALY();
			} catch (NotEnoughMoneyException&) {
				LOG_NOT_ENOUGH_MONEY(atmNumber, accountNumber, sum);
			} catch (...) {
				UNEXPECTED_EXCEPTION();
			}
		} else if (opCode == 'B') {//Check Balance
			try	{
				balance = bankAccountsMap.getAccountBalance(accountNumber, password);
				LOG_BALANCE(atmNumber, accountNumber, balance);
			} catch (WrongPasswordException&) {
				WRONG_PASSWORD_ILLEGALY();
			} catch (AccountDoesntExistException&) {
				ACCOUNT_DOESNT_EXIST_ILLEGALY();
			} catch (...) {
				UNEXPECTED_EXCEPTION();
			}
		}
		usleep(100000);
	}
	cmdList.close();//close file of commands
	pthread_exit(NULL);
	return NULL;
}

void* Commission_Run(void* cmds) {
	commissionThreadRunning = true;
	int commission, accountNumber, percentage;
	IteratorConst account;
	while(atmThreadRunning) {
		percentage = ((((rand())%(2)) + 2));//y = rand, x = (y(mod2)+2)/100
		for (account = bankAccountsMap.begin(); account != bankAccountsMap.end(); account++) {
			accountNumber = account->first;
			commission = bankAccountsMap.takeComission(accountNumber, percentage);
			pthread_mutex_lock(&bank_balance_lock);
			BankBalance += commission;
			pthread_mutex_unlock(&bank_balance_lock);
			LOG_COMMISSION(percentage, commission, accountNumber);
		}
		sleep(3); //Every 3 seconds commission is taken from all accounts
	}
	commissionThreadRunning = false;
	pthread_exit(NULL);
	return NULL;
}

void* Continuous_Print_Run(void* cmds) {
	int accountNumber;
	IteratorConst account;
	while(true) {//Continuous printing
		if (atmThreadRunning == false && commissionThreadRunning == false) {
			break;//ATMs threads are done and so is the bank commission thread
		}
		printf("\033[2J");
		printf("\033[1;1H");
		cout << "Current Bank Status" << endl;
		for (account = bankAccountsMap.begin(); account != bankAccountsMap.end(); account++) {
			accountNumber = account->first;
			bankAccountsMap.printAccountInMap(accountNumber);
		}
		pthread_mutex_lock(&bank_balance_lock);
		cout << "The Bank has " << BankBalance << " $" << endl;
		pthread_mutex_unlock(&bank_balance_lock);
		usleep(500000);
	}
	pthread_exit(NULL);
	return NULL;
}






