sparse_calc is a REPL designed to evaluate matrix and sparse matrix calculations.

Commands:
	- "matrix <operand>, ..."
		Creates a Matrix for each argument if name contains only
		letters and underscores. If a matrix with name argument is already
		saved in session, it will be overwritten.
	- "show <operand>, ..."
		Displays all listed argument matrices.
	- "list"
		Outputs the names and dimensions of all matrices in current session.
	- "drop <operand> ..."
		Remove argument matrices from current session.
	- "save <operand> ..."
		Saves matrices to persist across sessions. Matrix names must be
		unique and contain only letters and underscores.
	- "load <operand> ..."
		Loads previously saved matrices to current session. If matrix in
		current session as matrix being loaded, it will be overwritten.
	- "saved"
		Lists the names and dimensions of all saved matrices
	- "delete <operand> ..."
		Delete saved matrices"
	- "exec <operand> ..."
		Executes files at listed argument file path. Input files will be
		executed identically as if the lines were inputted one by one
		with the REPL, except that each statement must be closed with
		a semicolon.
	- "export <operand> ..."
		Export matrices to CSV format. CSV files will be saved as
		the name of the matrix followed by a timestamp.
	- "clear"
		Clears the current terminal screen.
	- "help"
		Displays basic info on how to use sparse_calc

Expression Evaluation

	sparse_calc can handle a variety of common linear algebra operations.
	Additionally, order of operations is handled during evaluation. Evaluation
	can produce a matrix, vector, or scalar. Expressions resulting in a scalar
	can be assigned to a specific index in a matrix. The element of matrix A
	at row x and column y can be accessed with "A[x][y]". This syntax can be
	applied both as the expression result and as an operand within the
	expression.

Supported Operations:
	- "+" Addition: Adds two matrices, a scalar and a matrix, or two scalars.
	- "-" Subtraction: Both operands can be either matrix or scalar. If first
	operand is a scalar and second is a matrix, every matrix element will be
	calculated as the scalar subtract itself.
	- "*" Multiplication: Multiplies two matrices, a scalar and a matrix, or
	two scalars.
	- "^" Exponent: Can raise a square matrix to a positive power. If the 
	square matrix has an inverse, it can be raised to a negative power.
	Matrices raised to non-integer powers is not supported. Additionally,
	A matrix cannot be an exponent. Scalars can be raised to any scalar
	power.
	- "'" Transpose: Performs a transpose of a matrix. Can be applied to
	a scalar but the value will not be modified.
	- "/" Division: A matrix or a scalar can be divided by any non-zero 
	scalar.
	- "()" Parentheses: Parentheses can be used to group operations.
	- "[]" Determinant: Calculates the determinant of a matrix. Square
	brackets are used instead of vertical bars to avoid ambiguity with
	nested determinants. When applied to a scalar the result will be
	itself. 
	

		
