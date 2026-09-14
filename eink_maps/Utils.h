#pragma once
#include <Arduino.h>

// --- FUNZIONE PER RIMUOVERE GLI ACCENTI ---
inline String replaceAccents(String str) {
  str.replace("à", "a'");
  str.replace("è", "e'");
  str.replace("é", "e'");
  str.replace("ì", "i'");
  str.replace("ò", "o'");
  str.replace("ù", "u'");
  
  str.replace("À", "A'");
  str.replace("È", "E'");
  str.replace("É", "E'");
  str.replace("Ì", "I'");
  str.replace("Ò", "O'");
  str.replace("Ù", "U'");
  
  return str;
}