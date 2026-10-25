/* Agenda da Mariana, preparada pelo Pi (~/accoes/app/agenda_estacao.py) e
 * servida pelo Cloudflare (https://estacao.ablabs.win/agenda.json, servidor.h).
 */

#ifndef __AGENDA_H__
#define __AGENDA_H__

#include <vector>
#include "ecra.h"

// true = agenda lida (pode vir vazia); false = servidor sem resposta, erro do
// Google ou ficheiro com mais de 3 h (o cron do Pi parou)
bool getAgenda(std::vector<dia_agenda_t> &agenda);

#endif
