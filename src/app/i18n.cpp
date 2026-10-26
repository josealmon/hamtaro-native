#include "app/i18n.h"
#include <string_view>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace hn {
namespace {
// Columns match the persisted language IDs: Spanish, English, French, German, Italian.
constexpr std::array<const char *,5> messages[]={
    {"Inicio","Home","Accueil","Startseite","Inizio"},
    {"Controles","Controls","Commandes","Steuerung","Comandi"},
    {"Pantalla y sonido","Video and audio","Affichage et son","Bild und Ton","Schermo e audio"},
    {"Jugar","Play","Jouer","Spielen","Gioca"},
    {"Continuar","Resume","Reprendre","Fortsetzen","Riprendi"},
    {"Reiniciar","Restart","Recommencer","Neustart","Ricomincia"},
    {"Volver al menú","Back to menu","Menu principal","Zum Menü","Torna al menu"},
    {"Seleccionar ROM","Select ROM","Choisir une ROM","ROM auswählen","Scegli ROM"},
    {"Cambiar ROM","Change ROM","Changer de ROM","ROM wechseln","Cambia ROM"},
    {"Europa (.gba)","Europe (.gba)","Europe (.gba)","Europa (.gba)","Europa (.gba)"},
    {"ROM lista","ROM ready","ROM prête","ROM bereit","ROM pronta"},
    {"Importar partida","Import save","Importer une sauvegarde","Spielstand importieren","Importa salvataggio"},
    {"Abrir carpeta","Open save folder","Ouvrir le dossier","Ordner öffnen","Apri cartella"},
    {"Sin mando conectado","No gamepad connected","Aucune manette connectée","Kein Controller verbunden","Nessun controller collegato"},
    {"Mando activo","Active gamepad","Manette active","Aktiver Controller","Controller attivo"},
    {"WASD como alternativa de movimiento","WASD movement alternative","Déplacement alternatif avec WASD","Alternativ mit WASD bewegen","Movimento alternativo con WASD"},
    {"Libera W, A, S y D en la tabla para activar esta opción.","Unassign W, A, S and D in the table to enable this option.","Libérez W, A, S et D dans le tableau pour activer cette option.","Entferne die Belegungen für W, A, S und D in der Tabelle, um diese Option zu aktivieren.","Rimuovi le assegnazioni di W, A, S e D nella tabella per attivare questa opzione."},
    {"Zona muerta del stick","Stick deadzone","Zone morte du stick","Stick-Totzone","Zona morta dello stick"},
    {"Pulsa un botón de la tabla para cambiarlo. Escape cancela.","Select a binding to change it. Escape cancels.","Choisissez une touche à modifier. Échap annule.","Zum Ändern eine Belegung wählen. Esc bricht ab.","Seleziona un comando da modificare. Esc annulla."},
    {"Acción GBA","GBA action","Action GBA","GBA-Aktion","Azione GBA"},
    {"Teclado","Keyboard","Clavier","Tastatur","Tastiera"},
    {"Mando","Gamepad","Manette","Controller","Controller"},
    {"Pulsa una tecla...","Press a key...","Appuyez sur une touche...","Taste drücken...","Premi un tasto..."},
    {"Pulsa un botón...","Press a button...","Appuyez sur un bouton...","Knopf drücken...","Premi un pulsante..."},
    {"Restablecer controles","Reset controls","Réinitialiser les commandes","Steuerung zurücksetzen","Ripristina comandi"},
    {"Idioma de la aplicación","Application language","Langue de l’application","Sprache der Anwendung","Lingua dell’applicazione"},
    {"Automático","Automatic","Automatique","Automatisch","Automatico"},
    {"Pantalla completa (F11)","Fullscreen (F11)","Plein écran (F11)","Vollbild (F11)","Schermo intero (F11)"},
    {"Escalado entero","Integer scaling","Échelle entière","Ganzzahlige Skalierung","Scala intera"},
    {"Pausar al cambiar de aplicación","Pause when switching applications","Mettre en pause en changeant d’application","Beim Wechsel der Anwendung pausieren","Pausa al cambio di applicazione"},
    {"Volumen","Volume","Volume","Lautstärke","Volume"},
    {"Silencio","Mute","Couper le son","Stumm","Disattiva audio"},
    {"No hay dispositivo de audio disponible.","No audio device is available.","Aucun périphérique audio n’est disponible.","Kein Audiogerät verfügbar.","Nessun dispositivo audio disponibile."},
    {"El juego ha llegado a una función pendiente del port. Puedes volver al menú. La última partida guardada se conserva.","The game reached an unfinished part of the port. You can return to the menu. Your last completed save is preserved.","Le jeu a atteint une fonction non encore adaptée. Vous pouvez revenir au menu. Votre dernière sauvegarde est conservée.","Das Spiel hat eine noch nicht angepasste Funktion erreicht. Du kannst zum Menü zurückkehren. Dein letzter Spielstand bleibt erhalten.","Il gioco ha raggiunto una funzione non ancora adattata. Puoi tornare al menu. L’ultimo salvataggio è conservato."},
    {"No se ha podido enviar el audio. Comprueba el dispositivo y pulsa Continuar.","Audio delivery failed. Check the output device and choose Resume.","Échec de la sortie audio. Vérifiez le périphérique et choisissez Reprendre.","Audioausgabe fehlgeschlagen. Prüfe das Ausgabegerät und wähle Fortsetzen.","Errore nell’uscita audio. Controlla il dispositivo e scegli Riprendi."},
    {"Vuelve al menú antes de cambiar la ROM o la partida.","Return to the menu before replacing the ROM or save.","Revenez au menu avant de remplacer la ROM ou la sauvegarde.","Kehre zum Menü zurück, bevor du die ROM oder den Spielstand ersetzt.","Torna al menu prima di sostituire la ROM o il salvataggio."},
    {"ROM importada y verificada.","ROM imported and verified.","ROM importée et vérifiée.","ROM importiert und überprüft.","ROM importata e verificata."},
    {"Partida importada. Se ha conservado la anterior cuando era válida.","Save imported. The previous valid save was backed up.","Sauvegarde importée. Une copie de la sauvegarde précédente valide a été conservée.","Spielstand importiert. Der vorherige gültige Spielstand wurde gesichert.","Salvataggio importato. È stata conservata una copia del precedente salvataggio valido."},
    {"No se modificarán los ajustes existentes. Los cambios de esta sesión no se guardarán.","Existing settings are preserved. Changes in this session will not be saved.","Les réglages existants sont conservés. Les modifications de cette session ne seront pas enregistrées.","Die vorhandenen Einstellungen bleiben erhalten. Änderungen dieser Sitzung werden nicht gespeichert.","Le impostazioni esistenti sono conservate. Le modifiche di questa sessione non verranno salvate."},
    {"ROM de Game Boy Advance","Game Boy Advance ROM","ROM Game Boy Advance","Game-Boy-Advance-ROM","ROM Game Boy Advance"},
    {"Partida GBA","GBA save","Sauvegarde GBA","GBA-Spielstand","Salvataggio GBA"},
    {"Derecha","Right","Droite","Rechts","Destra"},
    {"Izquierda","Left","Gauche","Links","Sinistra"},
    {"Arriba","Up","Haut","Oben","Su"},
    {"Abajo","Down","Bas","Unten","Giù"},
    {"Intro","Return","Entrée","Eingabe","Invio"},
    {"Espacio","Space","Espace","Leertaste","Spazio"},
    {"Retroceso","Backspace","Retour arrière","Rücktaste","Backspace"},
    {"Supr","Delete","Suppr","Entf","Canc"},
    {"Mayús izquierda","Left Shift","Maj gauche","Umschalt links","Maiusc sinistro"},
    {"Mayús derecha","Right Shift","Maj droite","Umschalt rechts","Maiusc destro"},
    {"Ctrl izquierda","Left Ctrl","Ctrl gauche","Strg links","Ctrl sinistro"},
    {"Ctrl derecha","Right Ctrl","Ctrl droit","Strg rechts","Ctrl destro"},
    {"Alt izquierda","Left Alt","Alt gauche","Alt links","Alt sinistro"},
    {"Alt derecha","Right Alt","Alt droit","Alt rechts","Alt destro"},
    {"Re Pág","PageUp","Page préc.","Bild auf","Pag su"},
    {"Av Pág","PageDown","Page suiv.","Bild ab","Pag giù"},
    {"Fin","End","Fin","Ende","Fine"},
    {"Inicio","Home key","Début","Pos1","Home"},
    {"Cruz","Cross","Croix","Kreuz","Croce"},
    {"Círculo","Circle","Cercle","Kreis","Cerchio"},
    {"Cuadrado","Square","Carré","Quadrat","Quadrato"},
    {"Triángulo","Triangle","Triangle","Dreieck","Triangolo"},
    {"A","a","A","A","A"},
    {"B","b","B","B","B"},
    {"X","x","X","X","X"},
    {"Y","y","Y","Y","Y"},
    {"Atrás","back","Retour","Zurück","Indietro"},
    {"Start","start","Start","Start","Start"},
    {"Guía","guide","Guide","Guide","Guida"},
    {"Stick izquierdo","leftstick","Stick gauche","Linker Stick","Stick sinistro"},
    {"Stick derecho","rightstick","Stick droit","Rechter Stick","Stick destro"},
    {"LB / L1","leftshoulder","LB / L1","LB / L1","LB / L1"},
    {"RB / R1","rightshoulder","RB / R1","RB / R1","RB / R1"},
    {"Cruceta arriba","dpup","Croix haut","Steuerkreuz oben","Croce su"},
    {"Cruceta abajo","dpdown","Croix bas","Steuerkreuz unten","Croce giù"},
    {"Cruceta izquierda","dpleft","Croix gauche","Steuerkreuz links","Croce sinistra"},
    {"Cruceta derecha","dpright","Croix droite","Steuerkreuz rechts","Croce destra"},
    {"No se puede abrir el archivo seleccionado.","Cannot open the selected file.","Impossible d’ouvrir le fichier sélectionné.","Die ausgewählte Datei kann nicht geöffnet werden.","Impossibile aprire il file selezionato."},
    {"El tamaño del archivo no es compatible.","File size is not supported.","La taille du fichier n’est pas compatible.","Die Dateigröße wird nicht unterstützt.","La dimensione del file non è supportata."},
    {"No se puede leer el archivo completo.","Cannot read the entire file.","Impossible de lire le fichier en entier.","Die Datei kann nicht vollständig gelesen werden.","Impossibile leggere l’intero file."},
    {"La ROM debe ocupar exactamente 16 MiB.","The ROM must be exactly 16 MiB.","La ROM doit faire exactement 16 Mio.","Die ROM muss genau 16 MiB groß sein.","La ROM deve essere di esattamente 16 MiB."},
    {"Se necesita la ROM europea AH3P, revisión 0.","This build needs the European AH3P revision 0 ROM.","La ROM européenne AH3P, révision 0, est requise.","Die europäische ROM AH3P, Revision 0, wird benötigt.","È necessaria la ROM europea AH3P, revisione 0."},
    {"La ROM no coincide. No se admiten ROM modificadas o dañadas.","The ROM hash does not match. Modified or damaged ROMs are not supported.","La ROM ne correspond pas. Les ROM modifiées ou endommagées ne sont pas prises en charge.","Die ROM-Prüfsumme stimmt nicht überein. Veränderte oder beschädigte ROMs werden nicht unterstützt.","La ROM non corrisponde. Le ROM modificate o danneggiate non sono supportate."},
    {"La partida no tiene el tamaño correcto. Una partida SRAM compatible ocupa 32768 bytes.","Invalid save size. A compatible SRAM save has 32768 bytes.","La taille de la sauvegarde est incorrecte. Une sauvegarde SRAM compatible contient 32768 octets.","Ungültige Spielstandgröße. Ein kompatibler SRAM-Spielstand hat 32768 Bytes.","Dimensione del salvataggio non valida. Un salvataggio SRAM compatibile contiene 32768 byte."},
    {"La partida existente está dañada. Restáurala o sustitúyela antes de guardar.","Existing save is damaged. Restore or replace it explicitly before saving.","La sauvegarde existante est endommagée. Restaurez-la ou remplacez-la avant de sauvegarder.","Der vorhandene Spielstand ist beschädigt. Stelle ihn vor dem Speichern wieder her oder ersetze ihn.","Il salvataggio esistente è danneggiato. Ripristinalo o sostituiscilo prima di salvare."},
    {"Se necesita un archivo .sav de 32768 bytes, no un estado rápido del emulador.","Expected a 32768-byte .sav file, not an emulator state.","Un fichier .sav de 32768 octets est requis, pas un état instantané d’émulateur.","Eine .sav-Datei mit 32768 Bytes wird benötigt, kein Emulator-Schnellspeicherstand.","È richiesto un file .sav di 32768 byte, non uno stato rapido dell’emulatore."},
    {"La copia de seguridad de la partida no es válida.","The save backup is not valid.","La copie de sauvegarde n’est pas valide.","Die Spielstandsicherung ist ungültig.","La copia di sicurezza del salvataggio non è valida."},
    {"Este perfil ya está abierto o no permite escribir.","This profile is already open, or is not writable.","Ce profil est déjà ouvert ou ne permet pas l’écriture.","Dieses Profil ist bereits geöffnet oder nicht beschreibbar.","Questo profilo è già aperto o non è scrivibile."},
    {"Los ajustes se guardaron con una versión no compatible.","Settings were written by an unsupported version.","Les réglages ont été enregistrés avec une version non compatible.","Die Einstellungen stammen aus einer nicht unterstützten Version.","Le impostazioni sono state salvate da una versione non supportata."},
    {"Se necesita una ROM AH3P, revisión 0, verificada.","A verified AH3P revision 0 ROM is required.","Une ROM AH3P, révision 0, vérifiée est requise.","Eine überprüfte ROM AH3P, Revision 0, wird benötigt.","È necessaria una ROM AH3P, revisione 0, verificata."},
    {"Detén la partida actual antes de iniciar otra.","Stop the current game before starting another.","Arrêtez la partie en cours avant d’en lancer une autre.","Beende das aktuelle Spiel, bevor du ein anderes startest.","Interrompi la partita attuale prima di avviarne un’altra."},
};
constexpr bool valid_catalog()
{
    for(std::size_t i=0;i<std::size(messages);i++) {
        for(const auto *text:messages[i])if(!text||!*text)return false;
        for(std::size_t j=0;j<i;j++)if(std::string_view(messages[i][1])==messages[j][1])return false;
    }
    return true;
}
static_assert(valid_catalog(),"Translations must be complete and have unique English keys.");
}
int language_from_windows_id(std::uint16_t id)
{
    switch(PRIMARYLANGID(id)) {
    case LANG_SPANISH:return 0;
    case LANG_FRENCH:return 2;
    case LANG_GERMAN:return 3;
    case LANG_ITALIAN:return 4;
    default:return 1;
    }
}
int system_language()
{
    static const int language=language_from_windows_id(GetUserDefaultUILanguage());
    return language;
}
const char *translate(int language,const char *english)
{
    if(language==automatic_language)language=system_language();
    if(language<0||language>=static_cast<int>(language_names.size()))language=1;
    for(const auto &row:messages)if(std::string_view(english)==row[1])return row[language];
    return english; // Native diagnostics and device/key names may not have a translation.
}
}
