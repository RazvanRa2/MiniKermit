<=== README - TEMA 1 - PC - RAZVAN RADOI - 321 CA ===>

	<== PREZENTARE GENERALA A IMPLEMENTARII ==>

		<= SENDER =>
		Sender trimite mai intai un pachet de tip S, pentru a ii spune lui
		reciever ca transmisia va incepe.	

		Apoi, pentru fiecare fisier, sender trimite numele acelui fisier.

		Se citesc, dupa, cate maxim 250 bytes din fisier, iar acestia sunt
		transmisi in cate un pachet.

		La terminarea transmiterii continutului unui fisier, se transmite un
		mesaj de tip EOF, printr-un pachet Z.

		Dupa ce s-au transmis toate fisierele, se transmite un pachet de tipul
		"end of transmision" (B).

		<= Reciever =>
		Pentru fiecare pachet pe care il primeste, reciever verifica mai 
		intai daca acesta este valid (prin crc).

		Daca pachetul e valid vede apoi care este tipul acelui pachet.

		Dupa tipul pachetului, face o anumita actiune, apoi confirma
		primirea acelui pachet.
		
		Daca pachetul nu e valid, trimite un NACK in speranta unei retransmiteri.

	<== Detalii de implementare ==>
	In aceasta sectiune voi prezenta acele parti din implementarea temei despre care simt
	ca nu au fost complet explicate prin comentarii in cod si care necesita 
	adaugiri.		

		<= RUTINA DE RETRANSMITERE =>
		Dupa ce sender trimite un pachet, asteapta sa primeasca un ack.
		Daca primeste un ack in 5 secunde, atunci totul e bine si frumos.
		Daca primeste un nack in 5 secunde, atunci retransmite, incrementand seq
		in pachet.
		Daca nu primeste nimic prima sau a doua oara, mai asteapta.
		Daca asteapta de 3 ori, mai mult de 5 secunde, atunci incheie transmisia.

		<= Validarea pachetelor =>
		Pentru a valida un pachet in reciever, calculez crc pe payload-ul trimis.
		Apoi, ma uit in payload, in pachetul MiniKermit, la crc-ul transmis de
		catre sender. Daca sunt egale, totul e bine. Daca nu sunt egale, 
		pachetul trebuie retransmis.

		<= Confirmarea pachetelor de tip S =>
		Daca se transmite un pachet de tip S, se activeaza un flag.
		Activarea acestui flag determina apelarea unei functii specifice.
		Aceasta functie creeaza un ACK special, conform textului temei.
		Dupa transmiterea acestui ack, flagul este pus pe false pentru ca
		pentru celelalte pachete, evident, trimitem ack-uri normale.


	<== Disclaimer ==>
	Rezolvarea temei ar fi putut fi modularizata mai mult, stiu. De exemplu puteam face
	niste metode de tipul sendPackageX care sa fie ca un wrapper peste cateva instructiuni.
	In acelasi timp, in metode de tipul getPackageX se repeta cateva instructiuni care
	adauga datele nemodificabile despre un pachet. 
	Cum totusi nu exista metode de peste 150 de linii si m-am straduit sa fie totul lizibil,
	sper ca e ok :)
