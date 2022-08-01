/* Author: David Konar (xkonar07)
 * E-mail: xkonar07@stud.fit.vutbr.cz
 * IPK - Project 2 - server
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <sstream>
#include <netdb.h>
#include <errno.h>
#include <iterator>
#include <unistd.h>
#include <regex.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>

using namespace std;

typedef struct params {
    int port;
} tParams, *ptrParams;



enum errors {
    E_OK,
    E_ARG,
    E_PORT,
    E_REGCOMP,
    E_REGADR,
    E_PROTOCOL,
    E_SOCKET,
    E_GETHOST,
    E_CON,
    E_CLOSE,
    E_WRITE,
    E_READ,
    E_ADDR,
    E_WRONG,
    E_FCLOSE,
};

const char *EMSG[] = {
    "VSE JE OK",                                       // E_OK
    "Spatne zadane parametry",                         // E_ARG
    "Spatny tvar portu",                               // E_URL
    "Chyba pri kompilace regexp",                      // E_REGCOMP
    "Zadana adresa neodpovida validnimu tvaru",        // E_REGADR
    "Spatny vstupni protokol",                         // E_PROTOCOL
    "Chyba pri vytvareni socketu",                     // E_SOCKET
    "Chyba pri prekladu jmena",                        // E_GETHOST
    "Chyba pri navazovani spojeni",                    // E_CON
    "Chyba pri uzavirani spojeni",                     // E_CLOSE
    "Chyba pri posilani GET pozadavku",                // E_WRITE
    "Chyba pri prijimani odpovedi",                    // E_READ
    "Chyba pri parsovani URI",                         // E_ADDR
    "Chyba pri importu",                               // E_WRONG
    "Chyba pri uzavirani souboru",                     // E_FCLOSE
};

void showHelp() {
    printf("Napoveda k projektu do predmetu IPK (Projekt 3)\n");
    printf("Autor: David Konar (xkonar07@stud.fit.vutbr.cz\n\n");
}



void showError(int error) {
    fprintf(stderr, "%s\n", EMSG[error]);
}


int processArgs(int argc, char** argv, tParams *par) {

    int i = 0;
    int c;

    // too many (few) arguments
    if(argc < 2 || argc > 3) {
        return EXIT_FAILURE;
    } else if (strcmp(argv[1], "-p") == 0) {
        par->port = atoi(argv[2]);
    } else
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void endProc(int status)
{
  waitpid(-1, &status, 0);
}

int dataExchange(int &t, string &fileName, string &action, int num, string &msg)
{
    string msgSend;
    string readLine;
    int file_start;
    int n = 0;
    int change = 0;
    int cnt = 0;

    FILE *fr = NULL;
    char buffer[1];

    ofstream mailFileWRITE;
    ifstream mailFileREAD;
    fstream mailFileBOTH;

    // -s argument; sending a message
    if(strcmp(action.c_str(),"SEND") == 0) {
        if(fopen(fileName.c_str(), "a") == NULL) {
            printf("Chyba pri zapisu do souboru\n");

            // vytvoreni hlavicky pozadavku
            msgSend.assign("DD 0.1 ");
            msgSend.append("FAIL");
            msgSend.append("\r\n\r\n");
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0)  //  odeslani odpovedi klientovi
            {
                showError(E_WRITE);
                return EXIT_FAILURE;
            }
        } else {
            // vytvoreni hlavicky pozadavku
            msgSend.assign("DD 0.1 ");
            msgSend.append("OK");
            msgSend.append("\r\n\r\n");
            msgSend.append("Ok.");
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0) { //  odeslani odpovedi klientovi
                showError(E_WRITE);
                return EXIT_FAILURE;
            }

            msg.append("\n");
            msg.append("<<#END_OF_MESSAGE#>>");
            msg.append("\n");

            mailFileWRITE.open (fileName.c_str(), ios::app);
            mailFileWRITE << msg.c_str();
            mailFileWRITE.close();
        }
    }
    // -d argument; deleting a message
    else if(strcmp(action.c_str(),"DEL") == 0) {
        if(fopen(fileName.c_str(), "a") == NULL) {
            msgSend.assign("DD 0.1 ");
            msgSend.append("FAIL");
            msgSend.append("\r\n\r\n");
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0) { //  odeslani odpovedi klientovi
                showError(E_WRITE);
                return EXIT_FAILURE;
            }

        } else {

            // vytvoreni hlavicky pozadavku
            msgSend.assign("DD 0.1 ");
            msgSend.append("OK");
            msgSend.append("\r\n\r\n");

            mailFileREAD.open (fileName.c_str());
            msg.clear();
            cnt = 1;
            while(mailFileREAD) {
                getline(mailFileREAD, readLine);
                if((readLine.find("<<#END_OF_MESSAGE#>>")) != string::npos) {
                    // predel zprav

                    if(cnt == num) {
                        //printf("- %s(%d)\n", readLine.c_str(),cnt);
                        change = 1;

                    }
                    else {
                       // printf("+ %s(%d)\n", readLine.c_str(),cnt);
                        msg.append(readLine);
                        msg.append("\n");
                    }
                    cnt++;
                } else {
                        if(cnt == num) {
                          //  printf("- %s(%d)\n", readLine.c_str(),cnt);
                          ;
                        }
                        else {
                           // printf("+ %s(%d)\n", readLine.c_str(),cnt);
                            msg.append(readLine);
                            msg.append("\n");
                        }
                }
            }
            mailFileREAD.close();

            // vratit data do souboru po vymazani radku
            mailFileWRITE.open (fileName.c_str());
            mailFileWRITE << msg.c_str();
            mailFileWRITE.close();

            // podle toho jestli opravdu k vymazu nebo bylo cislo mimo rozsah posli zpet navratovou informaci
            if(change)
                msgSend.append("Ok.");
            else
                msgSend.append("Err.");
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0)     {
                showError(E_WRITE);
                return EXIT_FAILURE;
            }
        }

    }
    // -r argument; reading mailbox
    else if(strcmp(action.c_str(),"READ") == 0) {
        if(fopen(fileName.c_str(), "r") == NULL) {

            msgSend.assign("DD 0.1 ");
            msgSend.append("EMPTY");
            msgSend.append("\r\n\r\n");
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0)    {
                showError(E_WRITE);
                return EXIT_FAILURE;
            }
        } else {
            // vytvoreni hlavicky pozadavku
            msgSend.assign("DD 0.1 ");
            msgSend.append("OK");
            msgSend.append("\r\n\r\n");

            int cnt = 1;
            mailFileREAD.open (fileName.c_str());
            while(mailFileREAD) {
                getline(mailFileREAD, readLine);
                if((readLine.find("<<#END_OF_MESSAGE#>>")) != string::npos) {

                    // predel zprav - citac poctu zprav daneho uzivatele
                    cnt++;
                    msgSend.append("\n");
                } else {
                    // printf("Pridavam: %s\n", readLine.c_str());
                    msgSend.append(readLine.c_str());
                }
            }

            // zavrit soubor; pridat koncovku pro odeslane data a odeslat
            mailFileREAD.close();
            msgSend.append("\r\n\r\n");

            if(write(t, msgSend.c_str(), msgSend.length()) < 0)     {
                showError(E_WRITE);
                return EXIT_FAILURE;
            }
        }
    }
    else  // uplne jiny pozadavek - NEZNAMY
    {

        // vytvoreni hlavicky pozadavku
        msgSend.assign("DD 0.1 ");
        msgSend.append("FAIL");
        msgSend.append("\r\n\r\n");
        msgSend.append("\r\n\r\n");

        if(write(t, msgSend.c_str(), msgSend.length()) < 0)    {
            showError(E_WRITE);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}


int connect(int port, tParams* par)
{
    struct sockaddr_in sin;
    struct hostent *hptr;
    sin.sin_family = PF_INET;
    sin.sin_port = htons(par->port);
    sin.sin_addr.s_addr = INADDR_ANY;
    socklen_t sinLen;

    string msg_rec;
    string fileName;
    string action;
    string msg;

    string tmp;
    char buf;
    int goOn = 1;
    int count = 0;
    int pid;
    int s;
    int t;
    int prev;
    int beginning;
    int ending;
    int status;
    int num = 0;


    signal(SIGCHLD, endProc);


    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {  // vytvoreni socketu
        showError(E_WRONG);
        return EXIT_FAILURE;
    }

    if(bind(s, (sockaddr *)&sin, sizeof(sin)) < 0 ) { // svaze IP a socket
        showError(E_WRONG);
        return EXIT_FAILURE;
    }


    if(listen(s, 5)) {
        showError(E_WRONG);
        return EXIT_FAILURE;
    }

    sinLen = sizeof(sin);
    while (1)  // cekani na pozavadky od klientu
    {
        msg_rec.clear();
        goOn = 1;
        if((t = accept(s, (struct sockaddr *)&sin, &sinLen)) < 0) { // vytvoreni socketu

            showError(E_WRONG);
            return EXIT_FAILURE;
        }

        pid = fork();  // vytvoreni noveho procesu
        if(pid < 0)  { // chyba pri vytboreni procesu
            showError(E_WRONG);
            kill(0, SIGTERM);
        }
        else if(pid == 0) {  // vznik potomka
            while(goOn == 1) {  // cteni dat od klienta
                if(read(t, &buf, 1) < 0)      {
                    showError(E_READ);
                    return EXIT_FAILURE;
                }
                msg_rec.append(&buf, 1);  // nacitani hlavicky

                if((buf == '\r') && prev == 1) {
                    goOn = 0;
                }
                if(buf == '\n') {
                    prev = 1;
                }
                else {
                    prev = 0;
                }
            }

            // prisel spravny pozadavek
            if(msg_rec.find("DD 0.1 CONNECT") != string::npos)     {
                // vyparsovani informaci z hlavicky
                beginning = msg_rec.find("ACTION:") + 7;
                ending = msg_rec.find("\r\n", beginning);
                action.assign(msg_rec, beginning, ending-beginning);

                beginning = msg_rec.find("USER:") + 5;
                ending = msg_rec.find("\r\n", beginning);
                fileName.assign(msg_rec, beginning, ending-beginning);

                beginning = msg_rec.find("NUM:") + 4;
                ending = msg_rec.find("\r\n", beginning);
                tmp.assign(msg_rec, beginning, ending-beginning);
                num = atoi(tmp.c_str());

                beginning = msg_rec.find("MSG:") + 4;
                ending = msg_rec.find("\r\n", beginning);
                msg.assign(msg_rec, beginning, ending-beginning);



                if(dataExchange(t, fileName, action, num, msg) == EXIT_FAILURE) {

                    return EXIT_FAILURE;
                }
            }
            // pozadavek nerozpoznan
            else {
                showError(E_WRONG);
                continue;
            }
            if(close(t) < 0) { // uzavreni socketu
                showError(E_CLOSE);
                return EXIT_FAILURE;
            }
            exit(EXIT_SUCCESS);
        }

        // kod rodicovskeho procesu
        if(close(t) < 0)  // uzavreni socketu
        {
            showError(E_CLOSE);
            return EXIT_FAILURE;
        }
    }
    if(close(s) < 0) { // uzavreni socketu
        showError(E_CLOSE);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


int main(int argc, char** argv) {
    int loop = 0;
    tParams myParams = {};
    int error = E_OK;

    if(processArgs(argc, argv, &myParams) == EXIT_FAILURE) {
        showError(E_ARG);
        return EXIT_FAILURE;
    }
    if(myParams.port == 0) {
        showError(E_PORT);
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////////////////
    // Connection
    if(connect(myParams.port, &myParams) == EXIT_FAILURE) {
        showError(error);
        return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

