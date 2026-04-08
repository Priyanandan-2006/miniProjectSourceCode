#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ACCOUNTS 100
#define DATA_FILE "credit.dat"
#define EXPORT_FILE "accounts.txt"
#define LOG_FILE "transactions.log"

struct clientData
{
    unsigned int acctNum;
    char lastName[15];
    char firstName[10];
    double balance;
};

unsigned int enterChoice(void);
void exportTextFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void listRecords(FILE *fPtr);
void searchRecord(FILE *fPtr);
void transferFunds(FILE *fPtr);
void showAnalytics(FILE *fPtr);
void displayLeaderboard(FILE *fPtr);
void customerInsights(FILE *fPtr);
void viewRecentTransactions(void);
void viewCustomerTransactionHistory(FILE *fPtr);

int openDataFile(FILE **fPtr);
int repairDataFileIfNeeded(FILE *fPtr);
void initializeBlankFile(FILE *fPtr);
int getAccountByNumber(FILE *fPtr, unsigned int account, struct clientData *client);
int writeAccount(FILE *fPtr, const struct clientData *client);
int clearAccount(FILE *fPtr, unsigned int account);
int readLine(char *buffer, size_t size);
int promptUnsignedInRange(const char *prompt, unsigned int minimum, unsigned int maximum, unsigned int *value);
int promptDoubleValue(const char *prompt, double *value);
void promptName(const char *prompt, char *buffer, size_t size);
void trimNewline(char *text);
void toLowerText(const char *source, char *target, size_t size);
void logTransaction(const char *action, const struct clientData *source, const struct clientData *target, double amount);
int compareClientsByBalanceDesc(const void *left, const void *right);

int main(void)
{
    FILE *cfPtr = NULL;
    unsigned int choice;

    if (!openDataFile(&cfPtr))
    {
        puts("Unable to open the data file.");
        return EXIT_FAILURE;
    }

    while ((choice = enterChoice()) != 13)
    {
        switch (choice)
        {
        case 1:
            exportTextFile(cfPtr);
            break;
        case 2:
            updateRecord(cfPtr);
            break;
        case 3:
            newRecord(cfPtr);
            break;
        case 4:
            deleteRecord(cfPtr);
            break;
        case 5:
            listRecords(cfPtr);
            break;
        case 6:
            searchRecord(cfPtr);
            break;
        case 7:
            transferFunds(cfPtr);
            break;
        case 8:
            showAnalytics(cfPtr);
            break;
        case 9:
            displayLeaderboard(cfPtr);
            break;
        case 10:
            customerInsights(cfPtr);
            break;
        case 11:
            viewRecentTransactions();
            break;
        case 12:
            viewCustomerTransactionHistory(cfPtr);
            break;
        default:
            puts("Incorrect choice. Please select a valid option.");
            break;
        }
    }

    puts("Thank you for choosing Titan Bank.");
    fclose(cfPtr);
    return EXIT_SUCCESS;
}

int openDataFile(FILE **fPtr)
{
    *fPtr = fopen(DATA_FILE, "rb+");

    if (*fPtr == NULL)
    {
        *fPtr = fopen(DATA_FILE, "wb+");
        if (*fPtr == NULL)
        {
            return 0;
        }

        initializeBlankFile(*fPtr);
        fflush(*fPtr);
        rewind(*fPtr);
    }

    if (!repairDataFileIfNeeded(*fPtr))
    {
        fclose(*fPtr);
        *fPtr = NULL;
        return 0;
    }

    return 1;
}

int repairDataFileIfNeeded(FILE *fPtr)
{
    long expectedSize = (long)(MAX_ACCOUNTS * sizeof(struct clientData));

    if (fseek(fPtr, 0, SEEK_END) != 0)
    {
        return 0;
    }

    long currentSize = ftell(fPtr);
    if (currentSize < 0)
    {
        return 0;
    }

    if (currentSize == expectedSize)
    {
        rewind(fPtr);
        return 1;
    }

    rewind(fPtr);

    struct clientData validRecords[MAX_ACCOUNTS];
    struct clientData blankClient = {0, "", "", 0.0};
    size_t loaded = 0;

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        validRecords[i] = blankClient;
    }

    while (loaded < MAX_ACCOUNTS)
    {
        struct clientData current;
        size_t result = fread(&current, sizeof(struct clientData), 1, fPtr);

        if (result != 1)
        {
            break;
        }

        if (current.acctNum >= 1 && current.acctNum <= MAX_ACCOUNTS)
        {
            validRecords[current.acctNum - 1] = current;
        }

        ++loaded;
    }

    rewind(fPtr);

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        if (fwrite(&validRecords[i], sizeof(struct clientData), 1, fPtr) != 1)
        {
            return 0;
        }
    }

    fflush(fPtr);
    rewind(fPtr);
    puts("Data file was repaired and normalized to 100 account slots.");
    return 1;
}

void initializeBlankFile(FILE *fPtr)
{
    struct clientData blankClient = {0, "", "", 0.0};

    rewind(fPtr);
    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);
    }
}

void exportTextFile(FILE *readPtr)
{
    FILE *writePtr = fopen(EXPORT_FILE, "w");
    struct clientData client;

    if (writePtr == NULL)
    {
        puts("Unable to create accounts.txt.");
        return;
    }

    rewind(readPtr);
    fprintf(writePtr, "%-6s %-15s %-10s %12s\n", "Acct", "Last Name", "First Name", "Balance");
    fprintf(writePtr, "------------------------------------------------\n");

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        if (fread(&client, sizeof(struct clientData), 1, readPtr) != 1)
        {
            break;
        }

        if (client.acctNum != 0)
        {
            fprintf(writePtr, "%-6u %-15s %-10s %12.2f\n",
                    client.acctNum, client.lastName, client.firstName, client.balance);
        }
    }

    fclose(writePtr);
    rewind(readPtr);
    printf("Formatted account snapshot stored in %s.\n", EXPORT_FILE);
}

void updateRecord(FILE *fPtr)
{
    unsigned int account;
    double transaction;
    struct clientData client;

    if (!promptUnsignedInRange("Enter account to update (1 - 100): ", 1, MAX_ACCOUNTS, &account))
    {
        return;
    }

    if (!getAccountByNumber(fPtr, account, &client) || client.acctNum == 0)
    {
        printf("Account #%u has no information.\n", account);
        return;
    }

    printf("Current: %-6u %-15s %-10s %12.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    if (!promptDoubleValue("Enter deposit (+) or withdrawal (-): ", &transaction))
    {
        return;
    }

    if (client.balance + transaction < 0)
    {
        puts("Transaction denied. Balance cannot go below zero.");
        return;
    }

    client.balance += transaction;

    if (!writeAccount(fPtr, &client))
    {
        puts("Failed to update account.");
        return;
    }

    logTransaction("ACCOUNT_UPDATE", &client, NULL, transaction);
    printf("Updated: %-6u %-15s %-10s %12.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);
}

void deleteRecord(FILE *fPtr)
{
    struct clientData client;
    unsigned int accountNum;

    if (!promptUnsignedInRange("Enter account number to delete (1 - 100): ", 1, MAX_ACCOUNTS, &accountNum))
    {
        return;
    }

    if (!getAccountByNumber(fPtr, accountNum, &client) || client.acctNum == 0)
    {
        printf("Account #%u does not exist.\n", accountNum);
        return;
    }

    if (!clearAccount(fPtr, accountNum))
    {
        puts("Failed to delete account.");
        return;
    }

    logTransaction("ACCOUNT_DELETE", &client, NULL, 0.0);
    printf("Account #%u deleted.\n", accountNum);
}

void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int accountNum;

    if (!promptUnsignedInRange("Enter new account number (1 - 100): ", 1, MAX_ACCOUNTS, &accountNum))
    {
        return;
    }

    if (!getAccountByNumber(fPtr, accountNum, &client))
    {
        puts("Unable to read account.");
        return;
    }

    if (client.acctNum != 0)
    {
        printf("Account #%u already contains information.\n", client.acctNum);
        return;
    }

    client.acctNum = accountNum;
    promptName("Enter last name: ", client.lastName, sizeof(client.lastName));
    promptName("Enter first name: ", client.firstName, sizeof(client.firstName));

    if (!promptDoubleValue("Enter opening balance: ", &client.balance))
    {
        return;
    }

    if (client.balance < 0)
    {
        puts("Opening balance cannot be negative.");
        return;
    }

    if (!writeAccount(fPtr, &client))
    {
        puts("Failed to create account.");
        return;
    }

    logTransaction("ACCOUNT_CREATE", &client, NULL, client.balance);
    printf("Account #%u created for %s %s.\n", client.acctNum, client.firstName, client.lastName);
}

void listRecords(FILE *fPtr)
{
    struct clientData client;
    int found = 0;

    rewind(fPtr);
    printf("\n%-6s %-15s %-10s %12s %s\n", "Acct", "Last Name", "First Name", "Balance", "Status");
    printf("-----------------------------------------------------------------\n");

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1)
        {
            break;
        }

        if (client.acctNum != 0)
        {
            const char *status = client.balance < 100.0 ? "LOW_BALANCE" : "HEALTHY";
            printf("%-6u %-15s %-10s %12.2f %s\n",
                   client.acctNum, client.lastName, client.firstName, client.balance, status);
            found = 1;
        }
    }

    rewind(fPtr);

    if (!found)
    {
        puts("No active accounts found.");
    }
}

void searchRecord(FILE *fPtr)
{
    char query[64];
    char queryLower[64];
    struct clientData client;
    int found = 0;

    printf("Search by account number, first name, or last name: ");
    if (!readLine(query, sizeof(query)))
    {
        return;
    }

    trimNewline(query);
    toLowerText(query, queryLower, sizeof(queryLower));
    rewind(fPtr);

    printf("\nSearch Results\n");
    printf("%-6s %-15s %-10s %12s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("---------------------------------------------------\n");

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        char firstLower[sizeof(client.firstName)];
        char lastLower[sizeof(client.lastName)];
        char accountText[16];

        if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1)
        {
            break;
        }

        if (client.acctNum == 0)
        {
            continue;
        }

        toLowerText(client.firstName, firstLower, sizeof(firstLower));
        toLowerText(client.lastName, lastLower, sizeof(lastLower));
        snprintf(accountText, sizeof(accountText), "%u", client.acctNum);

        if (strstr(firstLower, queryLower) != NULL ||
            strstr(lastLower, queryLower) != NULL ||
            strcmp(accountText, query) == 0)
        {
            printf("%-6u %-15s %-10s %12.2f\n",
                   client.acctNum, client.lastName, client.firstName, client.balance);
            found = 1;
        }
    }

    rewind(fPtr);

    if (!found)
    {
        puts("No matching accounts found.");
    }
}

void transferFunds(FILE *fPtr)
{
    unsigned int fromAccount;
    unsigned int toAccount;
    double amount;
    struct clientData sender;
    struct clientData receiver;

    if (!promptUnsignedInRange("Transfer from account (1 - 100): ", 1, MAX_ACCOUNTS, &fromAccount))
    {
        return;
    }

    if (!promptUnsignedInRange("Transfer to account (1 - 100): ", 1, MAX_ACCOUNTS, &toAccount))
    {
        return;
    }

    if (fromAccount == toAccount)
    {
        puts("Source and destination accounts must be different.");
        return;
    }

    if (!getAccountByNumber(fPtr, fromAccount, &sender) || sender.acctNum == 0)
    {
        puts("Source account does not exist.");
        return;
    }

    if (!getAccountByNumber(fPtr, toAccount, &receiver) || receiver.acctNum == 0)
    {
        puts("Destination account does not exist.");
        return;
    }

    if (!promptDoubleValue("Enter transfer amount: ", &amount))
    {
        return;
    }

    if (amount <= 0)
    {
        puts("Transfer amount must be positive.");
        return;
    }

    if (sender.balance < amount)
    {
        puts("Transfer denied. Insufficient balance.");
        return;
    }

    sender.balance -= amount;
    receiver.balance += amount;

    if (!writeAccount(fPtr, &sender) || !writeAccount(fPtr, &receiver))
    {
        puts("Transfer failed while writing account data.");
        return;
    }

    logTransaction("ACCOUNT_TRANSFER", &sender, &receiver, amount);
    printf("Transferred %.2f from #%u to #%u.\n", amount, fromAccount, toAccount);
}

void showAnalytics(FILE *fPtr)
{
    struct clientData client;
    struct clientData topClient = {0, "", "", 0.0};
    unsigned int activeAccounts = 0;
    unsigned int lowBalanceAccounts = 0;
    double totalBalance = 0.0;

    rewind(fPtr);

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1)
        {
            break;
        }

        if (client.acctNum == 0)
        {
            continue;
        }

        ++activeAccounts;
        totalBalance += client.balance;

        if (client.balance < 100.0)
        {
            ++lowBalanceAccounts;
        }

        if (topClient.acctNum == 0 || client.balance > topClient.balance)
        {
            topClient = client;
        }
    }

    rewind(fPtr);

    puts("\nPortfolio Analytics");
    puts("-------------------");
    printf("Active accounts      : %u\n", activeAccounts);
    printf("Total bank balance   : %.2f\n", totalBalance);
    printf("Average balance      : %.2f\n", activeAccounts ? totalBalance / activeAccounts : 0.0);
    printf("Low balance accounts : %u\n", lowBalanceAccounts);

    if (topClient.acctNum != 0)
    {
        printf("Top account          : #%u (%s %s) with %.2f\n",
               topClient.acctNum, topClient.firstName, topClient.lastName, topClient.balance);
    }
    else
    {
        puts("Top account          : No active accounts yet");
    }
}

void displayLeaderboard(FILE *fPtr)
{
    struct clientData rankedClients[MAX_ACCOUNTS];
    struct clientData client;
    int count = 0;

    rewind(fPtr);

    for (int i = 0; i < MAX_ACCOUNTS; ++i)
    {
        if (fread(&client, sizeof(struct clientData), 1, fPtr) != 1)
        {
            break;
        }

        if (client.acctNum != 0)
        {
            rankedClients[count++] = client;
        }
    }

    rewind(fPtr);

    if (count == 0)
    {
        puts("No active accounts available for leaderboard ranking.");
        return;
    }

    qsort(rankedClients, (size_t)count, sizeof(struct clientData), compareClientsByBalanceDesc);

    puts("\nTitan Bank Leaderboard");
    puts("----------------------");
    printf("%-6s %-15s %-10s %12s %s\n", "Rank", "Last Name", "First Name", "Balance", "Account");

    for (int i = 0; i < count && i < 5; ++i)
    {
        printf("%-6d %-15s %-10s %12.2f #%u\n",
               i + 1,
               rankedClients[i].lastName,
               rankedClients[i].firstName,
               rankedClients[i].balance,
               rankedClients[i].acctNum);
    }
}

void customerInsights(FILE *fPtr)
{
    unsigned int account;
    struct clientData client;
    double projectedBalance;
    const char *segment;
    const char *loanDecision;
    int wellnessScore;

    if (!promptUnsignedInRange("Enter account number for insights (1 - 100): ", 1, MAX_ACCOUNTS, &account))
    {
        return;
    }

    if (!getAccountByNumber(fPtr, account, &client) || client.acctNum == 0)
    {
        puts("Account not found.");
        return;
    }

    if (client.balance >= 5000.0)
    {
        segment = "PLATINUM";
        wellnessScore = 95;
    }
    else if (client.balance >= 2000.0)
    {
        segment = "GOLD";
        wellnessScore = 82;
    }
    else if (client.balance >= 500.0)
    {
        segment = "SILVER";
        wellnessScore = 68;
    }
    else
    {
        segment = "STARTER";
        wellnessScore = 45;
    }

    projectedBalance = client.balance * 1.05;
    loanDecision = client.balance >= 1000.0 ? "Eligible for micro-loan pre-check" : "Grow balance for better loan chances";

    puts("\nCustomer Insight Card");
    puts("---------------------");
    printf("Customer name        : %s %s\n", client.firstName, client.lastName);
    printf("Account number       : #%u\n", client.acctNum);
    printf("Current balance      : %.2f\n", client.balance);
    printf("Customer segment     : %s\n", segment);
    printf("Financial wellness   : %d/100\n", wellnessScore);
    printf("1-year projection    : %.2f at 5%% estimated growth\n", projectedBalance);
    printf("Loan insight         : %s\n", loanDecision);

    if (client.balance < 100.0)
    {
        puts("Smart alert          : Balance is critically low. Recommend a savings top-up.");
    }
    else if (client.balance < 500.0)
    {
        puts("Smart alert          : Moderate balance. Encourage automated monthly saving.");
    }
    else
    {
        puts("Smart alert          : Account is stable and performing well.");
    }
}

void viewRecentTransactions(void)
{
    FILE *logPtr = fopen(LOG_FILE, "r");
    char entries[50][256];
    char line[256];
    int count = 0;
    int start;

    if (logPtr == NULL)
    {
        puts("No transaction history available yet.");
        return;
    }

    while (fgets(line, sizeof(line), logPtr) != NULL)
    {
        strncpy(entries[count % 50], line, sizeof(entries[0]) - 1);
        entries[count % 50][sizeof(entries[0]) - 1] = '\0';
        ++count;
    }

    fclose(logPtr);

    if (count == 0)
    {
        puts("Transaction log is empty.");
        return;
    }

    puts("\nRecent Transactions");
    puts("-------------------");

    start = count > 5 ? count - 5 : 0;
    for (int i = start; i < count; ++i)
    {
        printf("%s", entries[i % 50]);
    }
}

void viewCustomerTransactionHistory(FILE *fPtr)
{
    FILE *logPtr;
    struct clientData client;
    unsigned int account;
    char line[256];
    char lastTransactionDate[32] = "No transactions yet";
    char statements[5][256];
    int statementCount = 0;
    double totalDeposits = 0.0;
    double totalWithdrawals = 0.0;
    double transfersSent = 0.0;
    double transfersReceived = 0.0;

    if (!promptUnsignedInRange("Enter account number for transaction history (1 - 100): ", 1, MAX_ACCOUNTS, &account))
    {
        return;
    }

    if (!getAccountByNumber(fPtr, account, &client) || client.acctNum == 0)
    {
        puts("Account not found.");
        return;
    }

    logPtr = fopen(LOG_FILE, "r");
    if (logPtr == NULL)
    {
        puts("No transaction history available yet.");
        return;
    }

    while (fgets(line, sizeof(line), logPtr) != NULL)
    {
        char timestamp[32] = "";
        char action[32] = "";
        unsigned int fromAccount = 0;
        unsigned int toAccount = 0;
        double amount = 0.0;
        char *timestampEnd = strchr(line, ']');
        char *actionStart = timestampEnd != NULL ? timestampEnd + 2 : line;
        char *fromPos = strstr(line, "from #");
        char *toPos = strstr(line, "to #");
        char *amountPos = strstr(line, "amount ");
        int matched = 0;

        if (timestampEnd != NULL)
        {
            size_t timestampLength = (size_t)(timestampEnd - line - 1);
            if (timestampLength >= sizeof(timestamp))
            {
                timestampLength = sizeof(timestamp) - 1;
            }

            memcpy(timestamp, line + 1, timestampLength);
            timestamp[timestampLength] = '\0';
        }

        if (sscanf(actionStart, "%31s", action) != 1)
        {
            continue;
        }

        if (fromPos != NULL)
        {
            sscanf(fromPos, "from #%u", &fromAccount);
        }

        if (toPos != NULL)
        {
            sscanf(toPos, "to #%u", &toAccount);
        }

        if (amountPos != NULL)
        {
            sscanf(amountPos, "amount %lf", &amount);
        }

        if (strcmp(action, "ACCOUNT_UPDATE") == 0 && fromAccount == account)
        {
            if (amount >= 0)
            {
                totalDeposits += amount;
            }
            else
            {
                totalWithdrawals += -amount;
            }
            matched = 1;
        }
        else if (strcmp(action, "ACCOUNT_TRANSFER") == 0)
        {
            if (fromAccount == account)
            {
                transfersSent += amount;
                matched = 1;
            }

            if (toAccount == account)
            {
                transfersReceived += amount;
                matched = 1;
            }
        }
        else if ((strcmp(action, "ACCOUNT_CREATE") == 0 || strcmp(action, "ACCOUNT_DELETE") == 0) && fromAccount == account)
        {
            matched = 1;
        }

        if (matched)
        {
            if (timestamp[0] != '\0' &&
                (strcmp(lastTransactionDate, "No transactions yet") == 0 || strcmp(timestamp, lastTransactionDate) > 0))
            {
                strcpy(lastTransactionDate, timestamp);
            }

            strncpy(statements[statementCount % 5], line, sizeof(statements[0]) - 1);
            statements[statementCount % 5][sizeof(statements[0]) - 1] = '\0';
            ++statementCount;
        }
    }

    fclose(logPtr);

    puts("\nCustomer Transaction History");
    puts("----------------------------");
    printf("Customer              : %s %s\n", client.firstName, client.lastName);
    printf("Account number        : #%u\n", client.acctNum);
    printf("Current balance       : %.2f\n", client.balance);
    printf("Total deposits        : %.2f\n", totalDeposits);
    printf("Total withdrawals     : %.2f\n", totalWithdrawals);
    printf("Transfers sent        : %.2f\n", transfersSent);
    printf("Transfers received    : %.2f\n", transfersReceived);
    printf("Last transaction date : %s\n", lastTransactionDate);

    puts("\nMini Statement");
    puts("--------------");

    if (statementCount == 0)
    {
        puts("No transactions found for this account.");
        return;
    }

    for (int i = statementCount > 5 ? statementCount - 5 : 0; i < statementCount; ++i)
    {
        printf("%s", statements[i % 5]);
    }
}

int getAccountByNumber(FILE *fPtr, unsigned int account, struct clientData *client)
{
    if (account < 1 || account > MAX_ACCOUNTS)
    {
        return 0;
    }

    if (fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET) != 0)
    {
        return 0;
    }

    if (fread(client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        return 0;
    }

    if (client->acctNum == account)
    {
        return 1;
    }

    if (client->acctNum == 0)
    {
        client->acctNum = 0;
        return 1;
    }

    client->acctNum = 0;
    return 1;
}

int writeAccount(FILE *fPtr, const struct clientData *client)
{
    unsigned int slot = client->acctNum;

    if (slot == 0)
    {
        return 0;
    }

    if (fseek(fPtr, (long)(slot - 1) * sizeof(struct clientData), SEEK_SET) != 0)
    {
        return 0;
    }

    if (fwrite(client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        return 0;
    }

    fflush(fPtr);
    return 1;
}

int clearAccount(FILE *fPtr, unsigned int account)
{
    struct clientData blankClient = {0, "", "", 0.0};

    if (account < 1 || account > MAX_ACCOUNTS)
    {
        return 0;
    }

    if (fseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET) != 0)
    {
        return 0;
    }

    if (fwrite(&blankClient, sizeof(struct clientData), 1, fPtr) != 1)
    {
        return 0;
    }

    fflush(fPtr);
    return 1;
}

unsigned int enterChoice(void)
{
    unsigned int menuChoice;
    puts("\nTITAN BANK");
    puts("\n   Smart Transaction Processing System");
    puts("1 - Export formatted text report");
    puts("2 - Deposit or withdraw from an account");
    puts("3 - Add a new account");
    puts("4 - Delete an account");
    puts("5 - List all active accounts");
    puts("6 - Search accounts");
    puts("7 - Transfer funds between accounts");
    puts("8 - Show analytics dashboard");
    puts("9 - Show top customers leaderboard");
    puts("10 - Show customer insight card");
    puts("11 - View recent transactions");
    puts("12 - View customer transaction history");
    puts("13 - End program");

    if (!promptUnsignedInRange("Enter your choice: ", 1, 13, &menuChoice))
    {
        return 0;
    }

    return menuChoice;
}

int readLine(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        return 0;
    }

    return 1;
}

int promptUnsignedInRange(const char *prompt, unsigned int minimum, unsigned int maximum, unsigned int *value)
{
    char input[64];
    char extra;
    unsigned int parsed;

    for (;;)
    {
        printf("%s", prompt);

        if (!readLine(input, sizeof(input)))
        {
            return 0;
        }

        if (sscanf(input, "%u %c", &parsed, &extra) == 1 && parsed >= minimum && parsed <= maximum)
        {
            *value = parsed;
            return 1;
        }

        printf("Please enter a number between %u and %u.\n", minimum, maximum);
    }
}

int promptDoubleValue(const char *prompt, double *value)
{
    char input[64];
    char extra;
    double parsed;

    for (;;)
    {
        printf("%s", prompt);

        if (!readLine(input, sizeof(input)))
        {
            return 0;
        }

        if (sscanf(input, "%lf %c", &parsed, &extra) == 1)
        {
            *value = parsed;
            return 1;
        }

        puts("Please enter a valid amount.");
    }
}

void promptName(const char *prompt, char *buffer, size_t size)
{
    for (;;)
    {
        printf("%s", prompt);

        if (!readLine(buffer, size))
        {
            buffer[0] = '\0';
            return;
        }

        trimNewline(buffer);

        if (strlen(buffer) > 0)
        {
            return;
        }

        puts("Value cannot be empty.");
    }
}

void trimNewline(char *text)
{
    size_t length = strlen(text);

    if (length > 0 && text[length - 1] == '\n')
    {
        text[length - 1] = '\0';
    }
}

void toLowerText(const char *source, char *target, size_t size)
{
    size_t i;

    if (size == 0)
    {
        return;
    }

    for (i = 0; source[i] != '\0' && i + 1 < size; ++i)
    {
        target[i] = (char)tolower((unsigned char)source[i]);
    }

    target[i] = '\0';
}

void logTransaction(const char *action, const struct clientData *source, const struct clientData *target, double amount)
{
    FILE *logPtr = fopen(LOG_FILE, "a");
    time_t now;
    struct tm *localTime;
    char timestamp[32];

    if (logPtr == NULL)
    {
        return;
    }

    now = time(NULL);
    localTime = localtime(&now);

    if (localTime == NULL || strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime) == 0)
    {
        strcpy(timestamp, "unknown-time");
    }

    fprintf(logPtr, "[%s] %s", timestamp, action);

    if (source != NULL)
    {
        fprintf(logPtr, " | from #%u %s %s | balance %.2f",
                source->acctNum, source->firstName, source->lastName, source->balance);
    }

    if (target != NULL)
    {
        fprintf(logPtr, " | to #%u %s %s | balance %.2f",
                target->acctNum, target->firstName, target->lastName, target->balance);
    }

    fprintf(logPtr, " | amount %.2f\n", amount);
    fclose(logPtr);
}

int compareClientsByBalanceDesc(const void *left, const void *right)
{
    const struct clientData *clientLeft = (const struct clientData *)left;
    const struct clientData *clientRight = (const struct clientData *)right;

    if (clientLeft->balance < clientRight->balance)
    {
        return 1;
    }

    if (clientLeft->balance > clientRight->balance)
    {
        return -1;
    }

    if (clientLeft->acctNum > clientRight->acctNum)
    {
        return 1;
    }

    if (clientLeft->acctNum < clientRight->acctNum)
    {
        return -1;
    }

    return 0;
}
