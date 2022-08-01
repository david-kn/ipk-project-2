/* Author: David Konar (xkonar07)
 * E-mail: xkonar07@stud.fit.vutbr.cz
 * IPK - Project 2 - client
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

using namespace std;

typedef struct params
{
    int port;
    string num;
    string action;
    string addr;
    string toWho;
    string msg;
} tParams, *ptrParams;

enum errors
{
    E_OK,
    E_ARG,
    E_URL,
    E_RESP,
    E_REGADR,
    E_PROTOCOL,
    E_SOCKET,
    E_GETHOST,
    E_CON,
    E_CLOSE,
    E_WRITE,
    E_READ,
    E_ADDR,

};

const char *EMSG[] = {
    "VSE JE OK",                                // E_OK
    "Spatne zadane parametry",                  // E_ARG
    "Chybny tvar URL",                          // E_URL
    "Spatny navratovy kod serveru",             // E_RESP
    "Zadana adresa neodpovida validnimu tvaru", // E_REGADR
    "Spatny vstupni protokol",                  // E_PROTOCOL
    "Chyba pri vytvareni socketu",              // E_SOCKET
    "Chyba pri prekladu jmena",                 // E_GETHOST
    "Chyba pri navazovani spojeni",             // E_CON
    "Chyba pri uzavirani spojeni",              // E_CLOSE
    "Chyba pri posilani GET pozadavku",         // E_WRITE
    "Chyba pri prijimani odpovedi",             // E_READ
    "Chyba pri parsovani URI",                  // E_ADDR
};

void showHelp()
{
    printf("Napoveda k projektu do predmetu IPK (Projekt 2)\n");
    printf("Autor: David Konar (xkonar07@stud.fit.vutbr.cz\n\n");
    printf("Uziti:\tclient HOST:PORT [-r UZIVATEL] [-s PRIJEMCE_ZPRAVA] [-d UZIVATEL CISLO]");
}

void showError(int error)
{
    fprintf(stderr, "%s\n", EMSG[error]);
}

int processArgs(int argc, char **argv, tParams *par)
{

    int i = 0;

    if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
        showHelp();
        exit(EXIT_SUCCESS); // nechat tak nebo prejit na RETURN ??????????!!!!!!!!!!!!!
                            // nechat tak nebo prejit na RETURN ??????????!!!!!!!!!!!!!
                            // nechat tak nebo prejit na RETURN ??????????!!!!!!!!!!!!!
                            // nechat tak nebo prejit na RETURN ??????????!!!!!!!!!!!!!
    }
    else if (argc >= 4 && argc <= 5)
    {
        for (i = 1; i < argc; i++)
        {
            if (i == 1)
            {
                par->addr.assign(argv[i]);
                continue;
            }
            if (i == 2)
            {
                if ((strcmp("-r", argv[i]) == 0) && argc == 4)
                {
                    par->action.assign("READ");
                    par->toWho.assign(argv[i + 1]);
                    break;
                }
                else if ((strcmp("-s", argv[i]) == 0) && argc == 5)
                {
                    par->action.assign("SEND");
                    par->toWho.assign(argv[i + 1]);
                    par->msg.assign(argv[i + 2]);
                    break;
                }
                else if ((strcmp("-d", argv[i]) == 0) && argc == 5)
                {
                    par->action.assign("DEL");
                    par->toWho.assign(argv[i + 1]);
                    par->num.assign(argv[i + 2]);
                    break;
                }
                else
                {
                    return EXIT_FAILURE;
                }
            }
        }
    }
    else
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int connectToServer(int *connectionSocket, tParams *par, int *error)
{

    struct sockaddr_in sin;
    struct hostent *hptr;
    sin.sin_family = PF_INET;
    sin.sin_port = htons(par->port);

    if (((*connectionSocket) = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        (*error) = E_SOCKET;
        return EXIT_FAILURE;
    }

    if ((hptr = gethostbyname(par->addr.c_str())) == NULL)
    {
        (*error) = E_GETHOST;
        return EXIT_FAILURE;
    }

    memcpy(&sin.sin_addr, hptr->h_addr, hptr->h_length);

    if (connect((*connectionSocket), (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        (*error) = E_CON;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int getData(int *connectionSocket, tParams *par, int *error)
{
    string response;
    string msg;
    char ch;
    char buf;
    int q2 = 0;
    int prev = 0;

    int goOn = 1;
    int count = 0;
    int dataSocket;
    int cnt = 0;

    stringstream s1;
    stringstream s2;
    s2 << par->num;

    // Tvoreni a odesilani pozadavku
    msg.assign("DD 0.1 ");
    msg.append("CONNECT");
    msg.append("\r\n");

    msg.append("ACTION:");
    msg.append(par->action.c_str());
    msg.append("\r\n");

    msg.append("USER:");
    msg.append(par->toWho.c_str());
    msg.append("\r\n");

    msg.append("NUM:");
    msg.append(s2.str());
    msg.append("\r\n");

    msg.append("MSG:");
    msg.append(par->msg.c_str());
    msg.append("\r\n\r\n");

    if (write((*connectionSocket), msg.c_str(), msg.length()) < 0)
    {
        (*error) = E_WRITE;
        return EXIT_FAILURE;
    }

    // cteni navratove hlavicky - kodu
    while (goOn == 1)
    {
        cnt++;
        if (read((*connectionSocket), &buf, 1) < 0)
        {
            (*error) = E_READ;
            return EXIT_FAILURE;
        }
        response.append(&buf, 1);

        if (isspace(buf) && prev == 1)
        {
            // odstrani posledni dva znaky \r\n - ktere jsou pro zobrazovani a zapis zbytecne
            response.erase(response.size() - 1);
            response.erase(response.size() - 1);
            goOn = 0;
        }
        if (buf == '\n')
        {
            prev = 1;
        }
        else
        {
            prev = 0;
        }
    }

    // kontroluje navratovou hlavicku
    if (response.find("FAIL") != string::npos)
    {
        showError(E_RESP);
        return EXIT_FAILURE;
    }
    if (response.find("EMPTY") != string::npos)
    {
        if (close((*connectionSocket)) < 0)
        {
            showError(E_CLOSE);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    response.clear();
    // cte data
    goOn = 1;
    while (goOn == 1)
    {
        cnt++;
        if (read((*connectionSocket), &buf, 1) < 0)
        {
            (*error) = E_READ;
            return EXIT_FAILURE;
        }
        response.append(&buf, 1);

        // pro pripad ze by na zacatku prebyvaly nejake volne radky - prvni dva znaky - smazat
        if (q2 < 2)
        {
            if (isspace(buf))
            {
                if (!response.empty())
                {
                    response.erase(response.size() - 1);
                }
            }
        }
        //printf("%c(%d)\n", buf, (int) buf);

        if (isspace(buf) && prev == 1)
        {
            // odstrani posledni dva znaky \r\n - ktere jsou pro zobrazovani a zapis zbytecne
            if (!response.empty())
            {
                response.erase(response.size() - 1);
            }
            goOn = 0;
        }
        if (buf == '\n')
        {
            prev = 1;
        }
        else
        {
            prev = 0;
        }
        q2++;
    }
    // pouze u rezimu READ se vypisuje na STDOUT, jinak na STDERR
    if ((strcmp(par->action.c_str(), "DEL") == 0) || (strcmp(par->action.c_str(), "SEND") == 0))
    {
        fprintf(stderr, "%s", response.c_str());
        ;
    }
    else
    {
        // vypis zprav ze schranky - jediny vystup na STDOUT
        printf("%s", response.c_str());
        ;
    }

    // uzavreni spojeni
    if (close((*connectionSocket)) < 0)
    {
        showError(E_CLOSE);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{

    tParams myParams = {};
    myParams.msg.clear();
    myParams.toWho.clear();

    int error = E_OK;
    char *p;
    int connectionSocket;
    string cp;

    if (processArgs(argc, argv, &myParams) == EXIT_FAILURE)
    {
        showError(E_ARG);
        return EXIT_FAILURE;
    }

    p = strtok(const_cast<char *>(myParams.addr.c_str()), ":");
    myParams.port = atoi(strtok(NULL, ":"));

    /////////////////////////////////////////////////////
    // Connection
    if (connectToServer(&connectionSocket, &myParams, &error) == EXIT_FAILURE)
    {
        showError(error);
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////////////////
    // Sending - Receiving data
    if (getData(&connectionSocket, &myParams, &error) == EXIT_FAILURE)
    {
        showError(error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
