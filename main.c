#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h> 
#include <fcntl.h>

#define I2C_FICHIER "/dev/i2c-1" // fichier Linux representant le BUS #2
#define I2C_ADRESSE 0x29 // adresse du Device I2C MPU-9250 (motion tracking)
// Registre et ID du modèle attendu pour le capteur VL6180X
#define VL6180X_ID_REGISTRE 0x000 
#define VL6180X_ID 0xb4         

typedef struct
{
    uint16_t reg;  // Registre à configurer
    uint8_t valeur; // Valeur de configuration
} VL6180X_Configuration;

// Configuration initiale des registres du capteur VL6180X
VL6180X_Configuration VL6810x_message[] = {
    {0x0207, 0x01}, {0x0208, 0x01}, {0x0096, 0x00}, {0x0097, 0xfd},
    {0x00e3, 0x00}, {0x00e4, 0x04}, {0x00e5, 0x02}, {0x00e6, 0x01},
    {0x00e7, 0x03}, {0x00f5, 0x02}, {0x00d9, 0x05}, {0x00db ,0xce},
    {0x00dc, 0x03}, {0x00dd, 0xf8}, {0x009f, 0x00}, {0x00a3, 0x3c},
    {0x00b7, 0x00}, {0x00bb, 0x3c}, {0x00b2, 0x09}, {0x00ca, 0x09},
    {0x0198, 0x01}, {0x01b0, 0x17}, {0x01ad, 0x00}, {0x00ff, 0x05},
    {0x0100, 0x05}, {0x0199, 0x05}, {0x01a6, 0x1b}, {0x01ac, 0x3e},
    {0x01a7, 0x1f}, {0x0030, 0x00}, {0x0011, 0x10}, {0x010a, 0x30},
    {0x003f, 0x46}, {0x0031, 0xFF}, {0x0040, 0x63}, {0x002e, 0x01},
    {0x001b, 0x09}, {0x003e, 0x31}, {0x0014, 0x24}, {0x0016, 0x00}
};

int initialise_VL6180X(int fd);
int lireDistance(int fd); 
int ecrireRegistre(int fd, uint16_t registre, uint8_t donnee); 
int lireRegistre(int fd, uint16_t registre, uint8_t *donnee); 


// Fonction pour initialiser le capteur VL6180X
int initialise_VL6180X(int fd)
{
    for (int i = 0; i < sizeof(VL6810x_message) / sizeof(VL6180X_Configuration); i++) 
	{
        uint8_t buffer[3] = { VL6810x_message[i].reg >> 8, 
                              VL6810x_message[i].reg & 0xFF, 
                              VL6810x_message[i].valeur };
        if (write(fd, buffer, 3) != 3)  // Écriture de chaque configuration dans le capteur
		{
            printf("Erreur: Configuration I2C\n");
            return -1;
        }
    }
    return 0;
}



int main() 
{
	// Ouverture du port I2C en lecture et écriture
   int fdPortI2C = open(I2C_FICHIER, O_RDWR);
    if (fdPortI2C == -1) 
	{
        printf("Erreur: Ouverture du port I2C\n");
        return -1;
    }

    // Configuration de l'adresse I2C du capteur
    if (ioctl(fdPortI2C, I2C_SLAVE, I2C_ADRESSE) < 0) 
	{
        printf("Erreur: Configuration de l'adresse I2C\n");
        close(fdPortI2C);
        return -1;
    }

    // Vérification du registre d'ID
    uint8_t registreID[2] = {0x00, 0x00}; // Adresse du registre d'identification
    uint8_t id_valeur;
    if (write(fdPortI2C, registreID, 2) != 2 || read(fdPortI2C, &id_valeur, 1) != 1) 
	{
        printf("Erreur: Lecture du registre d'ID\n");
        close(fdPortI2C);
        return -1;
    }
    if (id_valeur != VL6180X_ID)  // Vérifie si l'ID correspond
	{
        printf("ID incorrect: %#04x\n", id_valeur);
        close(fdPortI2C);
        return -1;
    }
    printf("ID correct: %#04x\n", id_valeur);

    // Initialisation du capteur
    if (initialise_VL6180X(fdPortI2C) != 0) 
	{
        close(fdPortI2C);
        return -1;
    }

    // Lecture de la distance
    if (lireDistance(fdPortI2C) != 0) 
	{
        close(fdPortI2C);
        return -1;
    }

// 	//Partie 2

// 	// Écriture et Lecture sur le port I2C
// 	uint8_t Source = 0x000; // registre d'ID du chip I2C
// 	uint8_t Destination;
// 	// Écriture et Lecture sur le port I2C
//     uint8_t registreID[2] = {0x00, 0x00}; // registre d'ID du chip I2C (MODEL_ID à 0x000)
//     uint8_t donnees[2]; // Pour stocker la valeur du registre ID
//     uint8_t NombreDOctetsALire = 2; // Nombre d'octets à lire
// 	uint8_t NombreDOctetsAEcrire = 2;


//    // Écriture de l'adresse du registre
//     if(write(fdPortI2C, registreID, sizeof(registreID)) != sizeof(registreID))
// 	{
//         printf("Erreur: Écriture I2C\n");
//         close(fdPortI2C);
//         return -1;
//     }

//     // Lecture de la valeur du registre ID
//     if (read(fdPortI2C, donnees, NombreDOctetsALire) != NombreDOctetsALire) 
// 	{
//         printf("Erreur: Lecture I2C\n");
//         close(fdPortI2C);
//         return -1;
//     }

//     printf("octets lus: %#04x %#04x\n", donnees[0], donnees[1]); // Affiche les octets lus
	close(fdPortI2C); /// Fermeture du 'file descriptor'
	return 0;
}


// Fonction pour écrire dans un registre spécifique du VL6180X
int ecrireRegistre(int fd, uint16_t registre, uint8_t donnee) 
{
    uint8_t buffer[3];
    buffer[0] = (registre >> 8) & 0xFF; // Octet supérieur de l'adresse
    buffer[1] = registre & 0xFF;        // Octet inférieur de l'adresse
    buffer[2] = donnee;                 // Donnée à écrire dans le registre
                                        
    if (write(fd, buffer, 3) != 3)   // Écriture dans le registre
	{
        printf("Erreur: Écriture dans le registre 0x%04X\n", registre);
        return -1;
    }
    return 0;
}

// Fonction pour lire un octet depuis un registre spécifique du VL6180X
int lireRegistre(int fd, uint16_t registre, uint8_t *donnee) 
{
    uint8_t buffer[2];
    buffer[0] = (registre >> 8) & 0xFF;  // MSB
    buffer[1] = registre & 0xFF;         // LSB
    
	// Envoie l'adresse du registre
    if (write(fd, buffer, 2) != 2) 
	{
        printf("Erreur: Écriture de l'adresse de registre\n");
        return -1;
    }
    
	// Lecture de la donnée du registre
	if (read(fd, donnee, 1) != 1) 
	{
        printf("Erreur: Lecture de la donnée\n");
        return -1;
    }
    return 0;
}


// Fonction pour lire la distance depuis le capteur VL6180X
int lireDistance(int fd) 
{
uint8_t start_range_cmd[3] = {0x00, 0x18, 0x01}; // Commande pour démarrer la mesure
    if (write(fd, start_range_cmd, 3) != 3) 
	{
        printf("Erreur: Écriture de début de mesure de distance\n");
        return -1;
    }

    // Pause pour attendre que la mesure soit terminée
    usleep(100000);

    uint8_t range_register[2] = {0x00, 0x62}; // Registre de résultat de la distance
    if (write(fd, range_register, 2) != 2) 
	{
        printf("Erreur: Écriture pour lecture de distance\n");
        return -1;
    }

    uint8_t distance;
    if (read(fd, &distance, 1) != 1) 
	{
        printf("Erreur: Lecture de la distance\n");
        return -1;
    }

    // Affiche la distance mesurée
    printf("Distance mesurée: %d mm\n", distance);
    return 0;
}





