/* Agenda da Mariana, preparada pelo Pi (~/accoes/app/agenda_estacao.py) e
 * servida só na rede de casa (http://rpi4.local:8090/agenda.json).
 */

#ifndef __AGENDA_H__
#define __AGENDA_H__

#include <vector>
#include "ecra.h"

// true = agenda lida (pode vir vazia); false = Pi não encontrado, erro do
// Google ou ficheiro com mais de 3 h (o cron do Pi parou)
bool getAgenda(std::vector<dia_agenda_t> &agenda);

#endif
