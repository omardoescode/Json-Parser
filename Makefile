run: main
	./main $(ARGS)

test: main
	@echo "Running all JSON tests..."
	@echo "========================"
	@for file in resources/*.json; do \
		echo "Testing: $$file"; \
		./main "$$file" || echo "FAILED: $$file"; \
	done
	@echo "========================"
	@echo "All tests completed!"

main: main.c
	gcc main.c -o main

.PHONY: test
