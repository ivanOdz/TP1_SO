all: app view slave

app: app.c
	gcc -Wall $< -g -o $@

view: view.c
	gcc -Wall $< -g -o $@

slave: slave.c
	gcc -Wall $< -g -o $@

clean:
	rm -f app view slave

.PHONY: all clean
