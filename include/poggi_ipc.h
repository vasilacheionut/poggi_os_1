#ifndef POGGI_IPC_H
#define POGGI_IPC_H

// Structura pentru cutia poștală a sistemului
typedef struct {
    int data;         // Mesajul transmis (un număr întreg în cazul nostru)
    int has_message;  // Flag: 1 dacă există un mesaj necitit, 0 dacă e goală
} PoggiMailbox;

// Inițializează sistemul IPC
void poggi_ipc_init(void);

// Trimite un mesaj în cutia poștală
int poggi_ipc_send(int value);

// Citește un mesaj din cutia poștală (returnează valoarea sau -1 dacă e goală)
int poggi_ipc_receive(int* out_value);

#endif // POGGI_IPC_H