/*
 * emumgr - emulation manager for GWENESIS on the Pico Held 2
 *
 * Copyright (C) 2023 Daniel Kammer (daniel.kammer@web.de)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
// Escape Code für Umlaute
// ä e4, Ä c4
// Ü dc, ü fc
// Ö d6, ö f6
// ß df

#pragma once

#define STR_MAIN_TITLE "Hauptmen\xfc"

#define STR_ENABLED "aktiviert"
#define STR_DISABLED "deaktiviert"
#define STR_OFF "Aus"
#define STR_ON "Ein"
#define STR_AUTO "automatisch"
#define STR_MANUAL "manuell"

#define STR_LOADING "Laden..."
#define STR_GO_BACK "\x3c""  Zur""\xfc""ck"
#define STR_BACK "Zur""\xfc""ck"

#define STR_ERROR "Fehler"
#define STR_ERR_OPEN_FILE "Datei kann nicht""\0""ge""\xf6""ffnet werden"
#define STR_ERR_READ_FILE "Datei kann nicht""\0""gelesen werden"
#define STR_ERR_WRITE_FILE "Datei kann nicht""\0""geschrieben werden"
#define STR_ERR_ROM "Beim Laden des ROMS"
#define STR_ERR_OPEN_DIR "Beim ""\xd6""ffnen des Ordners:"
#define STR_ERR_GAME_FIRST "Bitte zuerst ein""\0"" Spiel w""\xe4""hlen"
#define STR_ERR_SD_MOUNT "Beim Lesen der SD-Karte"
#define STR_ERR_NO_ROMS_FOUND "Keine ROMS gefunden in:"

#define STR_OPTIONS_MENU "Einstellungen"
#define STR_SAVE_STATE "Speichern"
#define STR_GAMESTATE_SAVED "Spielstand gesichert"
#define STR_LOAD_STATE "Laden"
#define STR_SEL_ROM "Spiel w\xe4hlen"
#define STR_SOUND "Lautst\xe4rke"
#define STR_BRIGHTNESS "Helligkeit"
#define STR_OPTIONS "Einstellungen"
#define STR_LAUNCH "Starten"
#define STR_SHUTDOWN "Ausschalten"
#define STR_ABOUT "Info"

#define STR_BAT "Batterieanzeige"
#define STR_FPS "FPS-Anzeige"
#define STR_VSYNC "Vsync erzwingen"
#define STR_FRAMEDROP_MODE "Framedrop-Modus"
#define STR_SUBSCREEN_MEM "Subscreen-Speicher"
#define STR_SUBSCREEN_PSRAM "PSRAM"
#define STR_SUBSCREEN_SHARED "geteilt"

#define STR_REGION "Region"
#define STR_PAL "PAL"
#define STR_NTSC "NTSC"

#define STR_ABOUT_TEXT "Snes9x on the Pico Held 2\0"\
                       " \0 "\
                       "github.com/fcipaq/snes9x_esp32\0"\
                       "2026 daniel.kammer@web.de\0"\
                       " \0 "\
                       "Dank:"\
                       " \0 "\
                       "All Snes9x collaborators\0"\
                       "github.com/ducalex/retro-go\0"\
                       " \0 "\
                       "Build:"\
                       //" \0  "

