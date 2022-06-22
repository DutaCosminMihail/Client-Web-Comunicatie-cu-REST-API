#include "helpers.h"
#include "parson.h"
#include "requests.h"
#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

int main(int argc, char *argv[]) {

  char *message;
  char *response;
  int sockfd;
  char *command = calloc(BUFLEN, sizeof(char));

  char **cookies = calloc(1, sizeof(char *));
  char *token = calloc(BUFLEN, sizeof(char));

  cookies[0] = calloc(BUFLEN, sizeof(char));

  char **body_form;
  body_form = calloc(1, sizeof(char *));

  body_form[0] = calloc(BUFLEN, sizeof(char));

  while (1) {

    sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);
    printf("Your command: ");
    fgets(command, BUFLEN, stdin);

    /* register - comanda de inregistrare cu request de tip POST
                - eroare in cazul in care user-ul este deja inregistrat
    */
    if (strcmp(command, "register\n") == 0) {

      // payload-ul contine username si parola
      char *username, *password;

      username = calloc(BUFLEN, sizeof(char));
      password = calloc(BUFLEN, sizeof(char));

      JSON_Value *value;
      value = json_value_init_object();

      JSON_Object *object;
      object = json_value_get_object(value);

      printf("username = ");
      fgets(username, BUFLEN, stdin);

      json_object_set_string(object, "username", username);

      printf("password = ");

      fgets(password, BUFLEN, stdin);
      json_object_set_string(object, "password", password);

      body_form[0] = json_serialize_to_string_pretty(value);

      // request de tip POST trimis catre server
      message =
          compute_post_request("34.241.4.235", "/api/v1/tema/auth/register",
                               "application/json", body_form, 1, NULL, 0, NULL);

      send_to_server(sockfd, message);
      // message - primit de la server
      response = receive_from_server(sockfd);

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
      } else
        printf("You registered successfully!\n");
      // eliberez din memorie payload-ul si obiectele create pentru parsarea
      // json-ului
      json_value_free(value);
      json_free_serialized_string(body_form[0]);
      json_value_free(response_value);
      continue;
    }

    /*
         exit - comanda prin care se iese din terminalul comenzilor
    */
    if (strcmp(command, "exit\n") == 0) {

      close_connection(sockfd);
      exit(0);
    }

    /*
         login - comanda prin care un user se authentifica in biblioteca
               - cod foarte asemanator cu cel al comenzii register
               - eroare daca credentialele sunt gresit introduse
               - intoarce cookie de sesiune
    */
    if (strcmp(command, "login\n") == 0) {

      // eroare daca se introduce de doua ori comanda <<login>>
      if (strstr(cookies[0], "connect")) {
        printf("You are already logged in\n");
        continue;
      }

      // payload -ul contine username si parola
      char *username, *password;

      username = calloc(BUFLEN, sizeof(char));
      password = calloc(BUFLEN, sizeof(char));

      JSON_Value *value;
      value = json_value_init_object();

      JSON_Object *object;
      object = json_value_get_object(value);

      printf("username = ");
      fgets(username, BUFLEN, stdin);

      json_object_set_string(object, "username", username);

      printf("password = ");

      fgets(password, BUFLEN, stdin);
      json_object_set_string(object, "password", password);

      body_form[0] = json_serialize_to_string_pretty(value);

      // request de tip POST trimis catre server
      message =
          compute_post_request("34.241.4.235", "/api/v1/tema/auth/login",
                               "application/json", body_form, 1, NULL, 0, NULL);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // parsarea cookie-ului din mesaj intr-un pointer
      char *begin;
      char *end;
      begin = strstr(response, "connect");
      end = strstr(response, "; Path");

      memcpy(cookies[0], begin, end - begin);

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
      } else {
        printf("You logged in successfully!\n");
      }

      // eliberez din memorie payload-ul si obiectele create pentru parsarea
      // json-ului
      json_value_free(value);
      json_free_serialized_string(body_form[0]);
      json_value_free(response_value);
      continue;
    }

    /*
         enter_library - comanda de cerere acces in biblioteca
                       - eroare in cazul in cazul in care userul nu este
       authentificat -intoarce un token JWT pentru a demonstra accesul la
       biblioteca
    */

    if (strcmp(command, "enter_library\n") == 0) {

      // eroare daca se introduce de doua ori comanda <<enter_library>>
      if (token[0]) {
        printf("You already entered library!\n");
        continue;
      }

      // request de tip GET trimis catre server
      message =
          compute_get_request("34.241.4.235", "/api/v1/tema/library/access",
                              NULL, cookies, 1, NULL);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // parsarea cookie-ului din mesaj intr-un pointer
      char *begin;
      char *end;
      begin = strstr(response, "connect");
      end = strstr(response, "; Path");

      memcpy(cookies[0], begin, end - begin);

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
        json_value_free(response_value);
        continue;
      }
      printf("You entered the library!\n");

      // parsez din json token-ul primit
      const char *requested_token =
          json_object_get_string(response_object, "token");
      strcpy(token, requested_token);
      json_value_free(response_value);
      continue;
    }

    /*
            get_books  - comanda de sumarizare a informatiilor despre cartile
       din biblioteca
                       - eroare in cazul in cazul in care userul nu are acces la
       biblioteca
                       - intoarce o lista de obiecte json
    */

    if (strcmp(command, "get_books\n") == 0) {

      char *library;

      // request de tip GET trimis catre server
      message = compute_get_request(
          "34.241.4.235", "/api/v1/tema/library/books", NULL, NULL, 0, token);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      library = strstr(response, "[");

      if (library) {

        puts(library);

        continue;
      }

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
      }

      json_value_free(response_value);
      continue;
    }

    /*
            add_book   - comanda prin care se adauga o carte in biblioteca
                       - *EROARE* in cazul in cazul in care userul nu are acces
       la biblioteca
                       - *EROARE* in cazul in care informatiile despre carte
       sunt incomplete/gresite
    */

    if (strcmp(command, "add_book\n") == 0) {

      /* payload-ul contine -titlu
                            -autor
                            -gen
                            -numarul paginilor
                            -publicatie
      */

      char *title;
      char *author;
      char *genre;
      char *page_count;
      char *publisher;

      title = calloc(BUFLEN, sizeof(char));
      author = calloc(BUFLEN, sizeof(char));
      genre = calloc(BUFLEN, sizeof(char));
      page_count = calloc(BUFLEN, sizeof(char));
      publisher = calloc(BUFLEN, sizeof(char));

      JSON_Value *value;
      value = json_value_init_object();

      JSON_Object *object;
      object = json_value_get_object(value);

      printf("title = ");

      fgets(title, BUFLEN, stdin);
      json_object_set_string(object, "title", title);

      printf("author = ");

      fgets(author, BUFLEN, stdin);
      json_object_set_string(object, "author", author);

      printf("genre = ");

      fgets(genre, BUFLEN, stdin);
      json_object_set_string(object, "genre", genre);

      printf("page_count = ");

      fgets(page_count, BUFLEN, stdin);
      json_object_set_number(object, "page_count", atoi(page_count));

      printf("publisher = ");

      fgets(publisher, BUFLEN, stdin);
      json_object_set_string(object, "publisher", publisher);

      // ne asiguram ca informatiile introduse sunt **CORECTE**
      if (strlen(title) > 1 && strlen(author) > 1 && strlen(genre) > 1 &&
          strlen(publisher) > 1 && strlen(page_count) > 0 &&
          atoi(page_count) > 0) {
        body_form[0] = json_serialize_to_string_pretty(value);

        // request de tip POST trimis catre server
        message = compute_post_request(
            "34.241.4.235", "/api/v1/tema/library/books", "application/json",
            body_form, 1, NULL, 0, token);

        send_to_server(sockfd, message);
        response = receive_from_server(sockfd);

        // generez prin parsarea json-ului mesajul de eroare daca exista
        JSON_Value *response_value;
        response_value = json_parse_string(strstr(response, "{"));

        JSON_Object *response_object;
        response_object = json_value_get_object(response_value);

        const char *error = json_object_get_string(response_object, "error");
        if (error) {

          puts(error);
        } else {
          printf("You added the book successfully!\n");
        }

        // eliberez din memorie payload-ul si obiectele create pentru parsarea
        // json-ului
        json_value_free(value);
        json_free_serialized_string(body_form[0]);
        json_value_free(response_value);

      } else {

        // mesajul de eroare la introducerea informatiilor gresite

        printf("Book with wrong format!\n Please try again!\n");
        json_value_free(value);
      }
      continue;
    }

    /*
            get_book   - comanda de sumarizare a informatiilor despre o carte
       dupa ID-ul dat
                       - *EROARE* in cazul in cazul in care userul nu are acces
       la biblioteca
                       - *EROARE* in cazul in care ID-ul este invalid
                       - *RASPUNS* cu lista de obiecte JSON [ID si titlu]
    */

    if (strcmp(command, "get_book\n") == 0) {

      // payload-ul raspunsului
      char *ID;
      char *path;
      char *book;
      ID = calloc(BUFLEN, sizeof(char));
      path = calloc(BUFLEN, sizeof(char));
      book = calloc(BUFLEN, sizeof(char));

      printf("ID = ");

      fgets(ID, BUFLEN, stdin);
      ID[strlen(ID) - 1] = 0;
      strcpy(path, "/api/v1/tema/library/books/");
      strcat(path, ID);

      message = compute_get_request("34.241.4.235", path, NULL, NULL, 0, token);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      book = strstr(response, "[");

      if (book) {

        puts(book);

        continue;
      }

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
      }

      json_value_free(response_value);
      continue;
    }

    /*
            delete_book- comanda de stergere a unei carti in functie de ID
                       - *EROARE* in cazul in cazul in care userul nu are acces
       la biblioteca
                       - *EROARE* in cazul in care ID-ul este invalid
    */

    if (strcmp(command, "delete_book\n") == 0) {

      char *ID;
      char *path;
      ID = calloc(BUFLEN, sizeof(char));
      path = calloc(BUFLEN, sizeof(char));

      printf("ID = ");
      fgets(ID, BUFLEN, stdin);
      ID[strlen(ID) - 1] = 0;
      strcpy(path, "/api/v1/tema/library/books/");
      strcat(path, ID);

      message =
          compute_delete_request("34.241.4.235", path, NULL, NULL, 0, token);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // generez prin parsarea json-ului mesajul de eroare daca exista
      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);
      } else {
        printf("You deleted the book successfully!\n");
      }

      json_value_free(response_value);
      continue;
    }
    /*
            logout     - comanda de dezabonare din biblioteca
                       - *EROARE* in cazul in cazul in care nuu se demonstreaza
       ca suntem dezabonati
    */

    if (strcmp(command, "logout\n") == 0) {

      message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout",
                                    NULL, cookies, 1, NULL);

      send_to_server(sockfd, message);
      response = receive_from_server(sockfd);

      // generez prin parsarea json-ului mesajul de eroare daca exista

      JSON_Value *response_value;
      response_value = json_parse_string(strstr(response, "{"));

      JSON_Object *response_object;
      response_object = json_value_get_object(response_value);

      const char *error = json_object_get_string(response_object, "error");
      if (error) {

        puts(error);

      } else {
        memset(cookies[0], 0, BUFLEN);
        memset(token, 0, BUFLEN);
        printf("You logged out successfully!\n");
      }

      json_value_free(response_value);

      continue;
    } else {

      // mesaj de eroare daca comanda a fost introdusa gresit
      printf("Command not found!\n");
    }
  }
}
