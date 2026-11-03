#include "localization.hpp"

#include <array>
#include <unordered_map>

namespace
{
    using Texts = std::array<const char*, 3>;

    const std::unordered_map<std::string, Texts>& catalog()
    {
        static const std::unordered_map<std::string, Texts> texts = {
            {"app.title", {"SWITCH IDENTITY MANAGER", "SWITCH IDENTITY MANAGER", "SWITCH IDENTITY MANAGER"}},
            {"menu.diagnostic", {"Diagnostic systeme (simulateur PC)", "System diagnostics (PC simulator)", "Diagnostico del sistema (simulador PC)"}},
            {"menu.prodinfo", {"Inspection PRODINFO (simulation)", "PRODINFO inspection (simulation)", "Inspeccion de PRODINFO (simulacion)"}},
            {"menu.backups", {"Sauvegardes personnelles (dossier local)", "Personal backups (local folder)", "Copias personales (carpeta local)"}},
            {"menu.integrity", {"Verification d'integrite", "Integrity check", "Verificacion de integridad"}},
            {"menu.report", {"Generer un rapport", "Generate a report", "Generar un informe"}},
            {"menu.settings", {"Configuration", "Settings", "Configuracion"}},
            {"menu.change_prodinfo", {"Modifier l'etat PRODINFO (simulation)", "Change PRODINFO status (simulation)", "Cambiar estado de PRODINFO (simulacion)"}},
            {"menu.create_backup", {"Creer une sauvegarde", "Create a backup", "Crear una copia"}},
            {"menu.restore_backup", {"Restaurer une sauvegarde", "Restore a backup", "Restaurar una copia"}},
            {"menu.exit", {"Quitter", "Exit", "Salir"}},
            {"prompt.selection", {"Selection : ", "Selection: ", "Seleccion: "}},
            {"message.closing", {"Fermeture de SIM.", "Closing SIM.", "Cerrando SIM."}},
            {"message.invalid_input", {"Entree invalide.", "Invalid input.", "Entrada no valida."}},
            {"message.unknown_option", {"Option inconnue.", "Unknown option.", "Opcion desconocida."}},
            {"prompt.continue", {"Appuie sur Entree pour continuer...", "Press Enter to continue...", "Pulsa Intro para continuar..."}},
            {"diagnostic.title", {"DIAGNOSTIC SYSTEME", "SYSTEM DIAGNOSTICS", "DIAGNOSTICO DEL SISTEMA"}},
            {"diagnostic.mode", {"Mode", "Mode", "Modo"}},
            {"diagnostic.console", {"Console cible", "Target console", "Consola objetivo"}},
            {"diagnostic.not_inspected", {"Nintendo Switch (non inspectee depuis le PC)", "Nintendo Switch (not inspected from PC)", "Nintendo Switch (no inspeccionada desde el PC)"}},
            {"diagnostic.hardware", {"Diagnostic materiel", "Hardware diagnostics", "Diagnostico de hardware"}},
            {"diagnostic.unavailable", {"Non disponible dans le simulateur PC", "Unavailable in the PC simulator", "No disponible en el simulador de PC"}},
            {"diagnostic.nand", {"Acces NAND", "NAND access", "Acceso a NAND"}},
            {"diagnostic.prodinfo", {"Acces PRODINFO", "PRODINFO access", "Acceso a PRODINFO"}},
            {"diagnostic.not_done", {"Non effectue", "Not performed", "No realizado"}},
            {"prodinfo.title", {"INSPECTION PRODINFO", "PRODINFO INSPECTION", "INSPECCION DE PRODINFO"}},
            {"prodinfo.mode", {"Mode : SIMULATION UNIQUEMENT", "Mode: SIMULATION ONLY", "Modo: SOLO SIMULACION"}},
            {"label.status", {"Statut", "Status", "Estado"}},
            {"prodinfo.intact_present", {"[OK] Donnees fictives presentes.", "[OK] Simulated data is present.", "[OK] Los datos simulados estan presentes."}},
            {"prodinfo.intact_consistent", {"[OK] Structure simulee coherente.", "[OK] Simulated structure is consistent.", "[OK] La estructura simulada es coherente."}},
            {"prodinfo.missing", {"[ERREUR] Donnees fictives absentes.", "[ERROR] Simulated data is missing.", "[ERROR] Faltan los datos simulados."}},
            {"prodinfo.no_data", {"[INFO] Aucune donnee a inspecter.", "[INFO] No data to inspect.", "[INFO] No hay datos que inspeccionar."}},
            {"prodinfo.corrupted", {"[ALERTE] Anomalies fictives detectees.", "[ALERT] Simulated anomalies detected.", "[ALERTA] Se detectaron anomalias simuladas."}},
            {"prodinfo.integrity_compromised", {"[INFO] Integrite simulee compromisee.", "[INFO] Simulated integrity is compromised.", "[INFO] La integridad simulada esta comprometida."}},
            {"prodinfo.masked", {"[INFO] Donnees fictives masquees.", "[INFO] Simulated data is masked.", "[INFO] Los datos simulados estan ocultos."}},
            {"prodinfo.masked_content", {"[INFO] Leur contenu n'est pas accessible dans cette simulation.", "[INFO] Their contents are unavailable in this simulation.", "[INFO] Su contenido no esta disponible en esta simulacion."}},
            {"integrity.title", {"VERIFICATION D'INTEGRITE", "INTEGRITY CHECKER", "COMPROBACION DE INTEGRIDAD"}},
            {"integrity.mode", {"MODE : SIMULATEUR PC", "MODE: PC SIMULATOR", "MODO: SIMULADOR DE PC"}},
            {"integrity.disclaimer", {"Score indicatif : il ne certifie pas l'etat reel de la console et ne garantit pas l'acces aux services Nintendo.", "Informational score: it does not certify the console's real condition or guarantee access to Nintendo services.", "Puntuacion informativa: no certifica el estado real de la consola ni garantiza el acceso a los servicios de Nintendo."}},
            {"integrity.points", {"Points", "Points", "Puntos"}},
            {"integrity.score", {"Score d'integrite", "Integrity score", "Puntuacion de integridad"}},
            {"integrity.verified_coverage", {"Couverture des controles verifies", "Verified checks coverage", "Cobertura de controles verificados"}},
            {"integrity.general_status", {"Etat general", "Overall status", "Estado general"}},
            {"integrity.excellent", {"EXCELLENT", "EXCELLENT", "EXCELENTE"}},
            {"integrity.good", {"BON", "GOOD", "BUENO"}},
            {"integrity.degraded", {"DEGRADE", "DEGRADED", "DEGRADADO"}},
            {"integrity.critical", {"CRITIQUE", "CRITICAL", "CRITICO"}},
            {"status.warning", {"WARNING", "WARNING", "ADVERTENCIA"}},
            {"status.error", {"ERROR", "ERROR", "ERROR"}},
            {"status.not_verified", {"NON VERIFIE", "NOT VERIFIED", "NO VERIFICADO"}},
            {"integrity.not_verified", {"CONTROLE NON VERIFIE", "CHECK NOT VERIFIED", "CONTROL NO VERIFICADO"}},
            {"backup.title", {"Sauvegardes personnelles dans ./backups/", "Personal backups in ./backups/", "Copias personales en ./backups/"}},
            {"backup.source_prompt", {"Chemin du fichier personnel a sauvegarder : ", "Path of the personal file to back up: ", "Ruta del archivo personal que se copiara: "}},
            {"backup.restore_name_prompt", {"Nom de la sauvegarde a restaurer : ", "Backup filename to restore: ", "Nombre de la copia que se restaurara: "}},
            {"backup.restore_destination_prompt", {"Chemin du nouveau fichier de destination (aucun ecrasement) : ", "Path for the new destination file (no overwrite): ", "Ruta del nuevo archivo de destino (sin sobrescribir): "}},
            {"backup.restore_source", {"Fichier source", "Source file", "Archivo de origen"}},
            {"backup.restore_destination", {"Destination", "Destination", "Destino"}},
            {"backup.restore_confirmation", {"Confirmez en saisissant", "Confirm by typing", "Confirma escribiendo"}},
            {"backup.restore_cancelled", {"Restauration annulee.", "Restore cancelled.", "Restauracion cancelada."}},
            {"restore.confirm_yes", {"OUI", "YES", "SI"}},
            {"backup.operation_error", {"Operation de sauvegarde echouee", "Backup operation failed", "Fallo en la operacion de copia"}},
            {"backup.error", {"Erreur", "Error", "Error"}},
            {"backup.add_files", {"Placez vos fichiers personnels dans ./backups/.", "Place your personal files in ./backups/.", "Coloca tus archivos personales en ./backups/."}},
            {"backup.none", {"Aucun fichier de sauvegarde trouve.", "No backup files found.", "No se encontraron archivos de copia."}},
            {"backup.readable", {"LISIBLE", "READABLE", "LEGIBLE"}},
            {"backup.empty", {"VIDE", "EMPTY", "VACIO"}},
            {"backup.ignored", {"entree(s) non-fichier ignoree(s).", "non-file entr(y/ies) ignored.", "entrada(s) que no es archivo ignorada(s)."}},
            {"backup.read_only", {"SHA-256 calculee en lecture seule. La verification exige un fichier de reference.", "SHA-256 computed read-only. Verification requires a reference file.", "SHA-256 calculada en solo lectura. La verificacion requiere un archivo de referencia."}},
            {"backup.hash", {"SHA-256", "SHA-256", "SHA-256"}},
            {"backup.reference_help", {"Fichier de reference : <nom-du-fichier>.sha256, contenant une seule somme SHA-256.", "Reference file: <filename>.sha256, containing one SHA-256 digest.", "Archivo de referencia: <nombre-de-archivo>.sha256, con una suma SHA-256."}},
            {"backup.more_files", {"Autres fichiers non affiches", "Additional files not shown", "Archivos adicionales no mostrados"}},
            {"backup.integrity.not_verified", {"NON VERIFIEE", "NOT VERIFIED", "NO VERIFICADA"}},
            {"backup.integrity.verified", {"VERIFIEE", "VERIFIED", "VERIFICADA"}},
            {"backup.integrity.mismatch", {"ECHEC", "MISMATCH", "NO COINCIDE"}},
            {"backup.integrity.invalid", {"SOMME INVALIDE", "INVALID CHECKSUM", "SUMA NO VALIDA"}},
            {"backup.integrity.error", {"ERREUR", "ERROR", "ERROR"}},
            {"backup.bytes", {"octets", "bytes", "bytes"}},
            {"report.path", {"Chemin du rapport TXT (Entree pour SIM-report.txt) : ", "TXT report path (Enter for SIM-report.txt): ", "Ruta del informe TXT (Intro para SIM-report.txt): "}},
            {"report.overwrite", {"Le fichier existe. Le remplacer ? (o/N) : ", "File exists. Replace it? (y/N): ", "El archivo ya existe. Reemplazarlo? (s/N): "}},
            {"report.overwrite_yes", {"o", "y", "s"}},
            {"report.export_error", {"Export impossible", "Export failed", "Error al exportar"}},
            {"report.exported", {"Rapport exporte", "Report exported", "Informe exportado"}},
            {"settings.title", {"CONFIGURATION", "SETTINGS", "CONFIGURACION"}},
            {"settings.current_language", {"Langue actuelle", "Current language", "Idioma actual"}},
            {"settings.choose_language", {"Choisir la langue : [1] Francais [2] Anglais [3] Espagnol [0] Retour", "Choose language: [1] French [2] English [3] Spanish [0] Back", "Elegir idioma: [1] Frances [2] Ingles [3] Espanol [0] Volver"}},
            {"settings.saved", {"Langue enregistree.", "Language saved.", "Idioma guardado."}},
            {"settings.save_error", {"Impossible d'enregistrer la configuration", "Could not save settings", "No se pudo guardar la configuracion"}},
            {"settings.unchanged", {"Langue inchangee.", "Language unchanged.", "Idioma sin cambios."}},
            {"settings.invalid", {"Choix de langue invalide.", "Invalid language choice.", "Seleccion de idioma no valida."}},
            {"settings.load_warning", {"Configuration invalide", "Invalid settings", "Configuracion no valida"}},
            {"language.french", {"Francais", "French", "Frances"}},
            {"language.english", {"Anglais", "English", "Ingles"}},
            {"language.spanish", {"Espagnol", "Spanish", "Espanol"}},
            {"report.title", {"SWITCH IDENTITY MANAGER - RAPPORT", "SWITCH IDENTITY MANAGER - REPORT", "SWITCH IDENTITY MANAGER - INFORME"}},
            {"report.score_notice", {"Ce score indicatif ne certifie pas l'etat reel de la console et ne garantit pas l'acces aux services Nintendo.", "This informational score does not certify the console's real condition or guarantee access to Nintendo services.", "Esta puntuacion informativa no certifica el estado real de la consola ni garantiza el acceso a los servicios de Nintendo."}},
            {"report.simulated_notice", {"Les controles systeme et PRODINFO sont simules dans le simulateur PC.", "System and PRODINFO checks are simulated in the PC simulator.", "Las comprobaciones del sistema y PRODINFO son simuladas en el simulador de PC."}},
            {"report.source", {"Source des informations", "Information source", "Fuente de informacion"}},
            {"report.integrity", {"VERIFICATION D'INTEGRITE", "INTEGRITY CHECK", "COMPROBACION DE INTEGRIDAD"}},
            {"report.score", {"Score", "Score", "Puntuacion"}},
            {"report.overall", {"Etat general", "Overall status", "Estado general"}},
            {"report.system", {"INSPECTION SYSTEME", "SYSTEM INSPECTION", "INSPECCION DEL SISTEMA"}},
            {"report.backups", {"SAUVEGARDES LOCALES (LECTURE SEULE)", "LOCAL BACKUPS (READ-ONLY)", "COPIAS LOCALES (SOLO LECTURA)"}},
            {"report.scan_error", {"Erreur d'analyse", "Scan error", "Error de analisis"}},
            {"report.no_backups", {"Aucun fichier de sauvegarde trouve.", "No backup files found.", "No se encontraron archivos de copia."}},
            {"report.read_check", {"Le SHA-256 est calcule; la verification depend d'une somme de reference .sha256.", "SHA-256 is computed; verification requires a reference .sha256 checksum.", "Se calcula SHA-256; la verificacion requiere una suma de referencia .sha256."}},
            {"report.ignored", {"Entrees non-fichier ignorees", "Non-file entries ignored", "Entradas que no son archivos ignoradas"}},
            {"prodinfo.state_title", {"ETAT PRODINFO", "PRODINFO STATUS", "ESTADO DE PRODINFO"}},
            {"prodinfo.state_intact", {"INTACT", "INTACT", "INTACTO"}},
            {"prodinfo.state_missing", {"MISSING", "MISSING", "AUSENTE"}},
            {"prodinfo.state_corrupted", {"CORRUPTED", "CORRUPTED", "DANADO"}},
            {"prodinfo.state_masked", {"MASKED", "MASKED", "OCULTO"}},
            {"prodinfo.state_changed", {"Nouvel etat", "New status", "Nuevo estado"}},
            {"switch.ui_title", {"SWITCH IDENTITY MANAGER", "SWITCH IDENTITY MANAGER", "SWITCH IDENTITY MANAGER"}},
            {"switch.ui_subtitle", {"Interface Switch - lecture seule", "Switch interface - read-only", "Interfaz Switch - solo lectura"}},
            {"switch.ui_hint", {"Haut/Bas : naviguer | A : choisir | B : retour", "Up/Down: navigate | A: select | B: back", "Arriba/Abajo: navegar | A: elegir | B: volver"}},
            {"switch.diagnostic_unavailable", {"Diagnostic materiel non disponible dans cette version.", "Hardware diagnostics are not available in this build.", "El diagnostico de hardware no esta disponible en esta version."}},
            {"switch.prodinfo_unavailable", {"Lecture PRODINFO non activee. Aucune donnee n'a ete lue.", "PRODINFO reading is not enabled. No data was read.", "La lectura de PRODINFO no esta habilitada. No se leyeron datos."}},
            {"switch.backup_unavailable", {"Gestion des sauvegardes non activee dans cette version.", "Backup management is not enabled in this build.", "La gestion de copias no esta habilitada en esta version."}},
            {"switch.no_nand_access", {"Aucun acces NAND ou PRODINFO n'est effectue.", "No NAND or PRODINFO access is performed.", "No se accede a NAND ni a PRODINFO."}},
            {"switch.press_b", {"Appuie sur B pour revenir au menu.", "Press B to return to the menu.", "Pulsa B para volver al menu."}},
            {"switch.language_changed", {"Langue modifiee.", "Language changed.", "Idioma cambiado."}},
            {"switch.firmware", {"Version du systeme", "System version", "Version del sistema"}},
            {"switch.firmware_error", {"Lecture de la version impossible", "Could not read system version", "No se pudo leer la version del sistema"}},
            {"switch.system_language", {"Langue de la console", "Console language", "Idioma de la consola"}},
            {"switch.language_error", {"Lecture de la langue impossible", "Could not read system language", "No se pudo leer el idioma del sistema"}},
            {"switch.service_error", {"Service systeme indisponible", "System service unavailable", "Servicio del sistema no disponible"}},
            {"switch.sd_backup_notice", {"Carte SD : /switch/SIM/backups. Analyse en lecture seule; les sommes de reference doivent etre fiables.", "SD card: /switch/SIM/backups. Read-only scan; reference checksums must be trusted.", "Tarjeta SD: /switch/SIM/backups. Analisis de solo lectura; las sumas de referencia deben ser fiables."}},
            {"switch.backup_path", {"Sauvegardes SD personnelles", "Personal SD backups", "Copias personales de la SD"}},
            {"switch.backup_actions", {"X : creer depuis un fichier de la carte SD. Y : restaurer vers un nouveau fichier; aucun ecrasement.", "X: create from an SD card file. Y: restore to a new file; no overwriting.", "X: crear desde un archivo de la SD. Y: restaurar en un archivo nuevo; sin sobrescritura."}},
            {"switch.keyboard_error", {"Clavier systeme indisponible.", "System keyboard unavailable.", "Teclado del sistema no disponible."}},
            {"switch.report_path_header", {"Nom du fichier de rapport (enregistre sur la carte SD)", "Report filename (saved on SD card)", "Nombre del informe (guardado en la tarjeta SD)"}},
            {"error.backup_source", {"Impossible d'inspecter le fichier source : ", "Could not inspect source file: ", "No se pudo inspeccionar el archivo de origen: "}},
            {"error.backup_create_dir", {"Impossible de creer le dossier de sauvegardes : ", "Could not create backup directory: ", "No se pudo crear el directorio de copias: "}},
            {"error.backup_create", {"Impossible de creer la sauvegarde : ", "Could not create backup: ", "No se pudo crear la copia: "}},
            {"error.backup_destination", {"Impossible d'inspecter la destination : ", "Could not inspect destination: ", "No se pudo inspeccionar el destino: "}},
            {"error.backup_destination_dir", {"Impossible d'inspecter le dossier de destination : ", "Could not inspect destination directory: ", "No se pudo inspeccionar el directorio de destino: "}},
            {"error.backup_restore", {"Impossible de restaurer la sauvegarde : ", "Could not restore backup: ", "No se pudo restaurar la copia: "}},
            {"error.backup_checksum_write", {"Impossible d'inspecter la somme de reference : ", "Could not inspect reference checksum: ", "No se pudo inspeccionar la suma de referencia: "}},
            {"switch.settings_directory_error", {"Impossible de creer le dossier de configuration sur la carte SD.", "Could not create settings directory on SD card.", "No se pudo crear la carpeta de configuracion en la tarjeta SD."}},
            {"error.settings_inspect", {"Impossible d'inspecter le fichier de configuration : ", "Could not inspect configuration file: ", "No se pudo inspeccionar el archivo de configuracion: "}},
            {"error.log_inspect", {"Impossible d'inspecter le fichier journal : ", "Could not inspect log file: ", "No se pudo inspeccionar el archivo de registro: "}},
            {"error.checksum_inspect", {"Impossible d'inspecter la somme de reference : ", "Could not inspect reference checksum: ", "No se pudo inspeccionar la suma de referencia: "}},
            {"backup.integrity", {"Integrite", "Integrity", "Integridad"}},
            {"switch.exit", {"Quitter", "Exit", "Salir"}},
            {"error.backup_inspect", {"Impossible d'inspecter le dossier de sauvegardes : ", "Could not inspect backup directory: ", "No se pudo inspeccionar la carpeta de copias: "}},
            {"error.backup_symlink", {"Le dossier de sauvegardes ne peut pas etre un lien symbolique.", "The backup directory cannot be a symbolic link.", "La carpeta de copias no puede ser un enlace simbolico."}},
            {"error.backup_invalid", {"Dossier de sauvegardes introuvable ou invalide.", "Backup directory is missing or invalid.", "La carpeta de copias no existe o no es valida."}},
            {"error.backup_list", {"Impossible de parcourir le dossier de sauvegardes : ", "Could not list backup directory: ", "No se pudo listar la carpeta de copias: "}},
            {"error.backup_entry", {"Impossible d'inspecter l'entree : ", "Could not inspect entry: ", "No se pudo inspeccionar la entrada: "}},
            {"error.backup_size", {"Impossible de lire la taille du fichier : ", "Could not read file size: ", "No se pudo leer el tamano del archivo: "}},
            {"error.backup_read", {"Erreur pendant la lecture du fichier.", "Error while reading the file.", "Error al leer el archivo."}},
            {"error.backup_access", {"Fichier inaccessible en lecture.", "File is not accessible for reading.", "No se puede leer el archivo."}},
            {"error.backup_iteration", {"Erreur pendant le parcours du dossier : ", "Error while iterating through directory: ", "Error al recorrer la carpeta: "}},
            {"error.readable", {"Lecture complete; le contenu n'a pas ete compare a une empreinte.", "Read completed; contents were not checked against a hash.", "Lectura completada; el contenido no se comparo con una huella."}},
            {"error.empty_file", {"Le fichier est vide.", "The file is empty.", "El archivo esta vacio."}},
            {"error.report_empty", {"Le chemin de sortie est vide.", "Output path is empty.", "La ruta de salida esta vacia."}},
            {"error.report_inspect", {"Impossible d'inspecter le fichier de sortie : ", "Could not inspect output file: ", "No se pudo inspeccionar el archivo de salida: "}},
            {"error.report_symlink", {"L'export vers un lien symbolique est refuse.", "Exporting to a symbolic link is refused.", "Se rechaza exportar a un enlace simbolico."}},
            {"error.report_not_file", {"Le chemin de sortie n'est pas un fichier regulier.", "Output path is not a regular file.", "La ruta de salida no es un archivo normal."}},
            {"error.report_exists", {"Le fichier existe deja; confirmez son remplacement.", "File already exists; confirm replacement.", "El archivo ya existe; confirma que quieres reemplazarlo."}},
            {"error.report_open", {"Impossible d'ouvrir le fichier de rapport en ecriture.", "Could not open report file for writing.", "No se pudo abrir el informe para escritura."}},
            {"error.report_write", {"Erreur pendant l'ecriture du rapport.", "Error while writing report.", "Error al escribir el informe."}},
            {"data.prodinfo", {"PRODINFO", "PRODINFO", "PRODINFO"}},
            {"data.system", {"SYSTEM DATA", "SYSTEM DATA", "DATOS DEL SISTEMA"}},
            {"data.backup", {"BACKUP", "BACKUP", "COPIA"}},
            {"data.configuration", {"CONFIGURATION", "CONFIGURATION", "CONFIGURACION"}},
            {"data.prodinfo_ok", {"Donnees simulees coherentes.", "Simulated data is consistent.", "Los datos simulados son coherentes."}},
            {"data.prodinfo_masked", {"Donnees simulees masquees.", "Simulated data is masked.", "Los datos simulados estan ocultos."}},
            {"data.prodinfo_missing", {"Donnees simulees absentes.", "Simulated data is missing.", "Faltan los datos simulados."}},
            {"data.prodinfo_corrupt", {"Anomalie simulee detectee.", "Simulated anomaly detected.", "Se detecto una anomalia simulada."}},
            {"data.system_consistent", {"Structure fictive coherente.", "Simulated structure is consistent.", "La estructura simulada es coherente."}},
            {"data.backup_available", {"Sauvegarde fictive disponible.", "Simulated backup is available.", "La copia simulada esta disponible."}},
            {"data.configuration_valid", {"Configuration fictive valide.", "Simulated configuration is valid.", "La configuracion simulada es valida."}}
        };
        return texts;
    }

    const std::unordered_map<std::string, Texts>& phraseTranslations()
    {
        static const std::unordered_map<std::string, Texts> translations = {
            {"SIMULATEUR PC", {"SIMULATEUR PC", "PC SIMULATOR", "SIMULADOR DE PC"}},
            {"Console cible", {"Console cible", "Target console", "Consola objetivo"}},
            {"Nintendo Switch (non inspectee depuis le PC)", {"Nintendo Switch (non inspectee depuis le PC)", "Nintendo Switch (not inspected from PC)", "Nintendo Switch (no inspeccionada desde el PC)"}},
            {"Diagnostic materiel", {"Diagnostic materiel", "Hardware diagnostics", "Diagnostico de hardware"}},
            {"Non disponible dans le simulateur PC", {"Non disponible dans le simulateur PC", "Unavailable in the PC simulator", "No disponible en el simulador de PC"}},
            {"Acces NAND", {"Acces NAND", "NAND access", "Acceso a NAND"}},
            {"Acces PRODINFO", {"Acces PRODINFO", "PRODINFO access", "Acceso a PRODINFO"}},
            {"Non effectue", {"Non effectue", "Not performed", "No realizado"}},
            {"PRODINFO", {"PRODINFO", "PRODINFO", "PRODINFO"}},
            {"SYSTEM DATA", {"DONNEES SYSTEME", "SYSTEM DATA", "DATOS DEL SISTEMA"}},
            {"BACKUP", {"SAUVEGARDE", "BACKUP", "COPIA"}},
            {"CONFIGURATION", {"CONFIGURATION", "CONFIGURATION", "CONFIGURACION"}},
            {"WARNING", {"AVERTISSEMENT", "WARNING", "ADVERTENCIA"}},
            {"ERROR", {"ERREUR", "ERROR", "ERROR"}},
            {"NOT VERIFIED", {"NON VERIFIE", "NOT VERIFIED", "NO VERIFICADO"}},
            {"EXCELLENT", {"EXCELLENT", "EXCELLENT", "EXCELENTE"}},
            {"BON", {"BON", "GOOD", "BUENO"}},
            {"DEGRADE", {"DEGRADE", "DEGRADED", "DEGRADADO"}},
            {"CRITIQUE", {"CRITIQUE", "CRITICAL", "CRITICO"}},
            {"Configuration directory must be on the SD card.", {"Le dossier de configuration doit etre sur la carte SD.", "Configuration directory must be on the SD card.", "La carpeta de configuracion debe estar en la tarjeta SD."}},
            {"Could not resolve configuration directory.", {"Impossible de resoudre le chemin du dossier de configuration.", "Could not resolve configuration directory.", "No se pudo resolver el directorio de configuracion."}},
            {"Configuration path cannot contain . or ..", {"Le chemin de configuration ne peut pas contenir . ou ..", "Configuration path cannot contain . or ..", "La ruta de configuracion no puede contener . o .."}},
            {"Could not inspect configuration directory.", {"Impossible d'inspecter le dossier de configuration.", "Could not inspect configuration directory.", "No se pudo inspeccionar el directorio de configuracion."}},
            {"Could not create configuration directory.", {"Impossible de creer le dossier de configuration.", "Could not create configuration directory.", "No se pudo crear el directorio de configuracion."}},
            {"Configuration directory cannot contain symbolic links.", {"Le dossier de configuration ne peut pas contenir de liens symboliques.", "Configuration directory cannot contain symbolic links.", "El directorio de configuracion no puede contener enlaces simbolicos."}},
            {"Configuration parent path is not a directory.", {"Un parent du chemin de configuration n'est pas un dossier.", "Configuration parent path is not a directory.", "Un directorio padre de la configuracion no es una carpeta."}},
            {"Configuration directory does not exist.", {"Le dossier de configuration n'existe pas.", "Configuration directory does not exist.", "El directorio de configuracion no existe."}},
            {"Le chemin doit se trouver sur la carte SD.", {"Le chemin doit se trouver sur la carte SD.", "Path must be on the SD card.", "La ruta debe estar en la tarjeta SD."}},
            {"La carte SD est indisponible ou invalide.", {"La carte SD est indisponible ou invalide.", "SD card is unavailable or invalid.", "La tarjeta SD no esta disponible o no es valida."}},
            {"Les chemins contenant . ou .. sont refuses.", {"Les chemins contenant . ou .. sont refuses.", "Paths containing . or .. are refused.", "Se rechazan las rutas que contienen . o .."}},
            {"Les liens symboliques sont refuses dans les chemins sensibles.", {"Les liens symboliques sont refuses dans les chemins sensibles.", "Symbolic links are refused in sensitive paths.", "Se rechazan los enlaces simbolicos en rutas sensibles."}},
            {"Source is not a regular file.", {"La source n'est pas un fichier regulier.", "Source is not a regular file.", "El origen no es un archivo normal."}},
            {"Could not read configuration file.", {"Impossible de lire le fichier de configuration.", "Could not read configuration file.", "No se pudo leer el archivo de configuracion."}},
            {"Could not open configuration file for writing.", {"Impossible d'ouvrir le fichier de configuration en ecriture.", "Could not open configuration file for writing.", "No se pudo abrir el archivo de configuracion para escritura."}},
            {"Error while writing configuration file.", {"Erreur d'ecriture du fichier de configuration.", "Error while writing configuration file.", "Error al escribir el archivo de configuracion."}},
            {"Configuration file cannot be a symbolic link.", {"Le fichier de configuration ne peut pas etre un lien symbolique.", "Configuration file cannot be a symbolic link.", "El archivo de configuracion no puede ser un enlace simbolico."}},
            {"Configuration path is not a regular file.", {"Le chemin de configuration n'est pas un fichier regulier.", "Configuration path is not a regular file.", "La ruta de configuracion no es un archivo normal."}},
            {"Log file cannot be a symbolic link.", {"Le fichier journal ne peut pas etre un lien symbolique.", "Log file cannot be a symbolic link.", "El archivo de registro no puede ser un enlace simbolico."}},
            {"Log path is not a regular file.", {"Le chemin du journal n'est pas un fichier regulier.", "Log path is not a regular file.", "La ruta del registro no es un archivo normal."}},
            {"Impossible d'ouvrir le fichier de rapport en ecriture.", {"Impossible d'ouvrir le fichier de rapport en ecriture.", "Could not open report file for writing.", "No se pudo abrir el informe para escritura."}},
            {"Erreur pendant l'ecriture du rapport.", {"Erreur pendant l'ecriture du rapport.", "Error while writing report.", "Error al escribir el informe."}},
            {"Le chemin de sortie est vide.", {"Le chemin de sortie est vide.", "Output path is empty.", "La ruta de salida esta vacia."}},
            {"L'export vers un lien symbolique est refuse.", {"L'export vers un lien symbolique est refuse.", "Exporting to a symbolic link is refused.", "Se rechaza exportar a un enlace simbolico."}},
            {"Le chemin de sortie n'est pas un fichier regulier.", {"Le chemin de sortie n'est pas un fichier regulier.", "Output path is not a regular file.", "La ruta de salida no es un archivo normal."}},
            {"Le fichier existe deja; confirmez son remplacement.", {"Le fichier existe deja; confirmez son remplacement.", "File already exists; confirm replacement.", "El archivo ya existe; confirma que quieres reemplazarlo."}},
            {"Aucune somme de reference; integrite non verifiee.", {"Aucune somme de reference; integrite non verifiee.", "No reference checksum; integrity not verified.", "No hay suma de referencia; integridad no verificada."}},
            {"La somme SHA-256 correspond.", {"La somme SHA-256 correspond.", "SHA-256 checksum matches.", "La suma SHA-256 coincide."}},
            {"La somme SHA-256 ne correspond pas.", {"La somme SHA-256 ne correspond pas.", "SHA-256 checksum does not match.", "La suma SHA-256 no coincide."}},
            {"Le fichier .sha256 doit contenir une seule somme SHA-256 de 64 caracteres hexadecimaux.", {"Le fichier .sha256 doit contenir une seule somme SHA-256 de 64 caracteres hexadecimaux.", "The .sha256 file must contain one 64-character hexadecimal SHA-256 digest.", "El archivo .sha256 debe contener una suma SHA-256 hexadecimal de 64 caracteres."}},
            {"Inspection PRODINFO indisponible; aucune donnee sensible n'est lue.", {"Inspection PRODINFO indisponible; aucune donnee sensible n'est lue.", "PRODINFO inspection is unavailable; no sensitive data is read.", "La inspeccion de PRODINFO no esta disponible; no se leen datos sensibles."}},
            {"Informations systeme lues; aucun controle d'integrite materielle n'est effectue.", {"Informations systeme lues; aucun controle d'integrite materielle n'est effectue.", "System information was read; no hardware integrity check was performed.", "Se leyo la informacion del sistema; no se realizo una comprobacion de integridad del hardware."}},
            {"Diagnostic materiel non effectue; controle non verifie.", {"Diagnostic materiel non effectue; controle non verifie.", "Hardware diagnostics were not performed; check not verified.", "No se realizo el diagnostico de hardware; control no verificado."}},
            {"Controle d'integrite materielle effectue.", {"Controle d'integrite materielle effectue.", "Hardware integrity check performed.", "Comprobacion de integridad del hardware realizada."}},
            {"Verification des sauvegardes impossible.", {"Verification des sauvegardes impossible.", "Backup verification failed.", "No se pudo verificar las copias."}},
            {"Aucune sauvegarde disponible.", {"Aucune sauvegarde disponible.", "No backups available.", "No hay copias disponibles."}},
            {"Toutes les sauvegardes ont une somme SHA-256 valide.", {"Toutes les sauvegardes ont une somme SHA-256 valide.", "All backups have a valid SHA-256 checksum.", "Todas las copias tienen una suma SHA-256 valida."}},
            {"Certaines sauvegardes ne sont pas verifiees.", {"Certaines sauvegardes ne sont pas verifiees.", "Some backups are not verified.", "Algunas copias no estan verificadas."}},
            {"Configuration valide chargee depuis le fichier.", {"Configuration valide chargee depuis le fichier.", "Valid configuration loaded from file.", "Configuracion valida cargada desde el archivo."}},
            {"Aucun fichier de configuration persistant.", {"Aucun fichier de configuration persistant.", "No persistent configuration file.", "No hay un archivo de configuracion persistente."}},
            {"Configuration invalide ou illisible.", {"Configuration invalide ou illisible.", "Configuration is invalid or unreadable.", "La configuracion no es valida o no se puede leer."}},
            {"La source ne peut pas etre un lien symbolique.", {"La source ne peut pas etre un lien symbolique.", "Source cannot be a symbolic link.", "El origen no puede ser un enlace simbolico."}},
            {"La source n'est pas un fichier regulier.", {"La source n'est pas un fichier regulier.", "Source is not a regular file.", "El origen no es un archivo normal."}},
            {"Nom de fichier source invalide.", {"Nom de fichier source invalide.", "Invalid source filename.", "Nombre de archivo de origen no valido."}},
            {"La sauvegarde existe deja; aucun fichier n'a ete remplace.", {"La sauvegarde existe deja; aucun fichier n'a ete remplace.", "Backup already exists; no file was replaced.", "La copia ya existe; no se reemplazo ningun archivo."}},
            {"La somme de reference existe deja.", {"La somme de reference existe deja.", "Reference checksum already exists.", "La suma de referencia ya existe."}},
            {"Impossible de calculer l'empreinte de la sauvegarde.", {"Impossible de calculer l'empreinte de la sauvegarde.", "Could not calculate backup checksum.", "No se pudo calcular la suma de la copia."}},
            {"Impossible d'ecrire la somme de reference de la sauvegarde.", {"Impossible d'ecrire la somme de reference de la sauvegarde.", "Could not write backup reference checksum.", "No se pudo escribir la suma de referencia de la copia."}},
            {"Erreur pendant l'ecriture de la somme de reference.", {"Erreur pendant l'ecriture de la somme de reference.", "Error writing reference checksum.", "Error al escribir la suma de referencia."}},
            {"Sauvegarde creee et somme SHA-256 enregistree.", {"Sauvegarde creee et somme SHA-256 enregistree.", "Backup created and SHA-256 checksum saved.", "Copia creada y suma SHA-256 guardada."}},
            {"Nom de sauvegarde invalide.", {"Nom de sauvegarde invalide.", "Invalid backup name.", "Nombre de copia no valido."}},
            {"Sauvegarde introuvable ou illisible.", {"Sauvegarde introuvable ou illisible.", "Backup not found or unreadable.", "Copia no encontrada o ilegible."}},
            {"La sauvegarde doit avoir une somme de reference valide avant restauration.", {"La sauvegarde doit avoir une somme de reference valide avant restauration.", "Backup must have a valid reference checksum before restoration.", "La copia debe tener una suma de referencia valida antes de restaurarse."}},
            {"Restauration non confirmee.", {"Restauration non confirmee.", "Restore was not confirmed.", "La restauracion no fue confirmada."}},
            {"La restauration doit rester dans le dossier SIM autorise.", {"La restauration doit rester dans le dossier SIM autorise.", "Restore destination must remain inside the allowed SIM folder.", "El destino de restauracion debe permanecer en la carpeta SIM permitida."}},
            {"La destination existe deja; aucun fichier n'a ete remplace.", {"La destination existe deja; aucun fichier n'a ete remplace.", "Destination already exists; no file was replaced.", "El destino ya existe; no se reemplazo ningun archivo."}},
            {"Le dossier de destination n'existe pas.", {"Le dossier de destination n'existe pas.", "Destination directory does not exist.", "El directorio de destino no existe."}},
            {"La verification de la copie restauree a echoue.", {"La verification de la copie restauree a echoue.", "Restored copy verification failed.", "Fallo la verificacion de la copia restaurada."}},
            {"Sauvegarde restauree apres verification SHA-256.", {"Sauvegarde restauree apres verification SHA-256.", "Backup restored after SHA-256 verification.", "Copia restaurada tras verificar SHA-256."}},
            {"Le fichier de somme ne peut pas etre un lien symbolique.", {"Le fichier de somme ne peut pas etre un lien symbolique.", "Checksum file cannot be a symbolic link.", "El archivo de suma no puede ser un enlace simbolico."}},
            {"La somme de reference n'est pas un fichier regulier.", {"La somme de reference n'est pas un fichier regulier.", "Reference checksum is not a regular file.", "La suma de referencia no es un archivo normal."}},
            {"Impossible de lire la taille du fichier.", {"Impossible de lire la taille du fichier.", "Could not read file size.", "No se pudo leer el tamano del archivo."}},
            {"Impossible d'ouvrir le fichier pour calculer son empreinte.", {"Impossible d'ouvrir le fichier pour calculer son empreinte.", "Could not open file to calculate its digest.", "No se pudo abrir el archivo para calcular su suma."}},
            {"Erreur de lecture pendant le calcul de l'empreinte.", {"Erreur de lecture pendant le calcul de l'empreinte.", "Read error while calculating digest.", "Error de lectura al calcular la suma."}},
            {"Fichier inaccessible ou erreur de lecture.", {"Fichier inaccessible ou erreur de lecture.", "File is inaccessible or a read error occurred.", "Archivo inaccesible o se produjo un error de lectura."}},
            {"Impossible d'inspecter la somme de reference : ", {"Impossible d'inspecter la somme de reference : ", "Could not inspect reference checksum: ", "No se pudo inspeccionar la suma de referencia: "}},
            {"Donnees simulees coherentes.", {"Donnees simulees coherentes.", "Simulated data is consistent.", "Los datos simulados son coherentes."}},
            {"Donnees simulees masquees.", {"Donnees simulees masquees.", "Simulated data is masked.", "Los datos simulados estan ocultos."}},
            {"Donnees simulees absentes.", {"Donnees simulees absentes.", "Simulated data is missing.", "Faltan los datos simulados."}},
            {"Anomalie simulee detectee.", {"Anomalie simulee detectee.", "Simulated anomaly detected.", "Se detecto una anomalia simulada."}},
            {"Structure fictive coherente.", {"Structure fictive coherente.", "Simulated structure is consistent.", "La estructura simulada es coherente."}},
            {"Sauvegarde fictive disponible.", {"Sauvegarde fictive disponible.", "Simulated backup is available.", "La copia simulada esta disponible."}},
            {"Configuration fictive valide.", {"Configuration fictive valide.", "Simulated configuration is valid.", "La configuracion simulada es valida."}},
            {"Donnees simulees coherentes", {"Donnees simulees coherentes", "Simulated data is consistent", "Los datos simulados son coherentes"}},
            {"Donnees simulees masquees", {"Donnees simulees masquees", "Simulated data is masked", "Los datos simulados estan ocultos"}},
            {"Donnees simulees absentes", {"Donnees simulees absentes", "Simulated data is missing", "Faltan los datos simulados"}},
            {"Anomalie simulee detectee", {"Anomalie simulee detectee", "Simulated anomaly detected", "Se detecto una anomalia simulada"}},
            {"Structure fictive coherente", {"Structure fictive coherente", "Simulated structure is consistent", "La estructura simulada es coherente"}},
            {"Sauvegarde fictive disponible", {"Sauvegarde fictive disponible", "Simulated backup is available", "La copia simulada esta disponible"}},
            {"Configuration fictive valide", {"Configuration fictive valide", "Simulated configuration is valid", "La configuracion simulada es valida"}}
        };
        return translations;
    }

    std::size_t languageIndex(Language language)
    {
        switch (language)
        {
            case Language::FRENCH:
                return 0;
            case Language::ENGLISH:
                return 1;
            case Language::SPANISH:
                return 2;
        }
        return 0;
    }
}

Localization::Localization(Language language)
    : currentLanguage(language)
{
}

Language Localization::language() const
{
    return currentLanguage;
}

std::string Localization::languageCode() const
{
    switch (currentLanguage)
    {
        case Language::FRENCH:
            return "fr";
        case Language::ENGLISH:
            return "en";
        case Language::SPANISH:
            return "es";
    }
    return "fr";
}

std::string Localization::languageName() const
{
    switch (currentLanguage)
    {
        case Language::FRENCH:
            return text("language.french");
        case Language::ENGLISH:
            return text("language.english");
        case Language::SPANISH:
            return text("language.spanish");
    }
    return text("language.french");
}

std::string Localization::text(const std::string& key) const
{
    const auto found = catalog().find(key);
    if (found == catalog().end())
    {
        return key;
    }
    return found->second[languageIndex(currentLanguage)];
}

std::string Localization::translate(const std::string& frenchText) const
{
    const auto found = phraseTranslations().find(frenchText);
    if (found != phraseTranslations().end())
    {
        const std::size_t index = languageIndex(currentLanguage);
        const auto textFound = catalog().find(frenchText);
        if (textFound != catalog().end())
        {
            return textFound->second[index];
        }
        return found->second[index];
    }

    static const std::array<std::pair<const char*, const char*>, 18> prefixes = {{
        {"Impossible d'inspecter le dossier de sauvegardes : ", "error.backup_inspect"},
        {"Impossible de creer le dossier de sauvegardes : ", "error.backup_create_dir"},
        {"Impossible de parcourir le dossier de sauvegardes : ", "error.backup_list"},
        {"Impossible d'inspecter l'entree : ", "error.backup_entry"},
        {"Impossible de lire la taille du fichier : ", "error.backup_size"},
        {"Erreur pendant le parcours du dossier : ", "error.backup_iteration"},
        {"Impossible d'inspecter le fichier source : ", "error.backup_source"},
        {"Impossible de creer la sauvegarde : ", "error.backup_create"},
        {"Impossible d'inspecter la destination : ", "error.backup_destination"},
        {"Impossible d'inspecter le dossier de destination : ", "error.backup_destination_dir"},
        {"Impossible de restaurer la sauvegarde : ", "error.backup_restore"},
        {"Impossible d'inspecter la somme de reference : ", "error.backup_checksum_write"},
        {"Impossible d'inspecter le fichier de sortie : ", "error.report_inspect"},
        {"Could not inspect configuration file: ", "error.settings_inspect"},
        {"Could not inspect log file: ", "error.log_inspect"},
        {"Erreur : ", "backup.error"},
        {"Dossier de sauvegardes introuvable ou invalide.", "error.backup_invalid"},
        {"Le dossier de sauvegardes ne peut pas etre un lien symbolique.", "error.backup_symlink"}
    }};
    for (const auto& prefix : prefixes)
    {
        const std::string sourcePrefix = prefix.first;
        if (frenchText.compare(0, sourcePrefix.size(), sourcePrefix) == 0)
        {
            return text(prefix.second)
                 + frenchText.substr(sourcePrefix.size());
        }
    }

    if (frenchText == "OK")
    {
        return "OK";
    }
    if (frenchText == "Fichier inaccessible en lecture.")
    {
        return text("error.backup_access");
    }
    if (frenchText == "Erreur pendant la lecture du fichier.")
    {
        return text("error.backup_read");
    }
    if (frenchText == "Le fichier est vide.")
    {
        return text("error.empty_file");
    }
    if (frenchText == "Lecture complete; le contenu n'a pas ete compare a une empreinte.")
    {
        return text("error.readable");
    }
    return frenchText;
}

std::string Localization::translateSystemLanguage(
    const std::string& languageCode
) const
{
    if (languageCode == "en")
    {
        return text("language.english");
    }
    if (languageCode == "es")
    {
        return text("language.spanish");
    }
    if (languageCode == "fr")
    {
        return text("language.french");
    }
    return languageCode;
}
