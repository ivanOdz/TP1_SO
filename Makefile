all: app view slave

app: app.c
	gcc -Wall $< -o $@

view: view.c
	gcc -Wall $< -o $@

slave: slave.c
	gcc -Wall $< -o $@

clean:
	rm -f app view slave

.PHONY: all clean
