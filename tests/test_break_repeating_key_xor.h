#ifndef TEST_BREAK_REPEATING_KEY_XOR_H
#define TEST_BREAK_REPEATING_KEY_XOR_H
/*
* test_break_repeating_key_xor.h
*/


/* see challenge-data/6.txt */
static const char *s1c6_ciphertext_base64 =
	"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";


static const char *s1c6_key = "Terminator X: Bring the noise";


static const char *s1c6_plaintext =
	"I'm back and I'm ringin' the bell \n"
	"A rockin' on the mike while the fly girls yell \n"
	"In ecstasy in the back of me \n"
	"Well that's my DJ Deshay cuttin' all them Z's \n"
	"Hittin' hard and the girlies goin' crazy \n"
	"Vanilla's on the mike, man I'm not lazy. \n"
	"\n"
	"I'm lettin' my drug kick in \n"
	"It controls my mouth and I begin \n"
	"To just let it flow, let my concepts go \n"
	"My posse's to the side yellin', Go Vanilla Go! \n"
	"\n"
	"Smooth 'cause that's the way I will be \n"
	"And if you don't give a damn, then \n"
	"Why you starin' at me \n"
	"So get off 'cause I control the stage \n"
	"There's no dissin' allowed \n"
	"I'm in my own phase \n"
	"The girlies sa y they love me and that is ok \n"
	"And I can dance better than any kid n' play \n"
	"\n"
	"Stage 2 -- Yea the one ya' wanna listen to \n"
	"It's off my head so let the beat play through \n"
	"So I can funk it up and make it sound good \n"
	"1-2-3 Yo -- Knock on some wood \n"
	"For good luck, I like my rhymes atrocious \n"
	"Supercalafragilisticexpialidocious \n"
	"I'm an effect and that you can bet \n"
	"I can take a fly girl and make her wet. \n"
	"\n"
	"I'm like Samson -- Samson to Delilah \n"
	"There's no denyin', You can try to hang \n"
	"But you'll keep tryin' to get my style \n"
	"Over and over, practice makes perfect \n"
	"But not if you're a loafer. \n"
	"\n"
	"You'll get nowhere, no place, no time, no girls \n"
	"Soon -- Oh my God, homebody, you probably eat \n"
	"Spaghetti with a spoon! Come on and say it! \n"
	"\n"
	"VIP. Vanilla Ice yep, yep, I'm comin' hard like a rhino \n"
	"Intoxicating so you stagger like a wino \n"
	"So punks stop trying and girl stop cryin' \n"
	"Vanilla Ice is sellin' and you people are buyin' \n"
	"'Cause why the freaks are jockin' like Crazy Glue \n"
	"Movin' and groovin' trying to sing along \n"
	"All through the ghetto groovin' this here song \n"
	"Now you're amazed by the VIP posse. \n"
	"\n"
	"Steppin' so hard like a German Nazi \n"
	"Startled by the bases hittin' ground \n"
	"There's no trippin' on mine, I'm just gettin' down \n"
	"Sparkamatic, I'm hangin' tight like a fanatic \n"
	"You trapped me once and I thought that \n"
	"You might have it \n"
	"So step down and lend me your ear \n"
	"'89 in my time! You, '90 is my year. \n"
	"\n"
	"You're weakenin' fast, YO! and I can tell it \n"
	"Your body's gettin' hot, so, so I can smell it \n"
	"So don't be mad and don't be sad \n"
	"'Cause the lyrics belong to ICE, You can call me Dad \n"
	"You're pitchin' a fit, so step back and endure \n"
	"Let the witch doctor, Ice, do the dance to cure \n"
	"So come up close and don't be square \n"
	"You wanna battle me -- Anytime, anywhere \n"
	"\n"
	"You thought that I was weak, Boy, you're dead wrong \n"
	"So come on, everybody and sing this song \n"
	"\n"
	"Say -- Play that funky music Say, go white boy, go white boy go \n"
	"play that funky music Go white boy, go white boy, go \n"
	"Lay down and boogie and play that funky music till you die. \n"
	"\n"
	"Play that funky music Come on, Come on, let me hear \n"
	"Play that funky music white boy you say it, say it \n"
	"Play that funky music A little louder now \n"
	"Play that funky music, white boy Come on, Come on, Come on \n"
	"Play that funky music \n";

#endif /* ndef TEST_BREAK_REPEATING_KEY_XOR_H */
