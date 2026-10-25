/* Servidor da estação no Pi, pelo Cloudflare (SERVIDOR no config.cpp), com o
 * Service Token do segredos.h: agenda da Mariana e atualizações por Wi-Fi.
 */

#ifndef __SERVIDOR_H__
#define __SERVIDOR_H__

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// prepara o pedido https://SERVIDOR<caminho>; depois é só http.GET()
void pedirServidor(WiFiClientSecure &client, HTTPClient &http,
                   const char *caminho);

// atualização por Wi-Fi: se o servidor tiver uma versão maior do que
// VERSAO_FIRMWARE, instala-a e reinicia (não volta); senão volta logo
void verificarAtualizacao();

#endif
