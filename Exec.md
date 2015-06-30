Executes a Windows process and returns information about its execution.

## Arguments ##

| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Command Line | String   | Specifies the command line to execute. |
| Wait     | Boolean  | Determines whether exec will wait for the process to terminate before returning. |
| Hide     | Boolean  | Shows or hides any windows created by the child process. |
| Batch    | Boolean  | Specifies whether the command line is a batch file. |
| Capture  | Boolean  | Specifies whether exec should capture what's written to standard out/error. |

## Return Value ##

If the function succeeds it returns a JavaScript object with the return code of the process and the captured output of stdout/stderror if requested. The properties of the object are exitCode, stdOut, and stdErr, and will all be defined regardless of whether a capture was performed.

If there is an error, it will return false.

## Remarks ##

Only the command line is a required parameter. If you do not pass any of the optional boolean values, the function will assume you want created windows to be hidden, that you want to wait for termination, that it's not a batch file, and that you do want to capture standard outputs.

If exec is called with the wait parameter set to false, it will only return true - no exit code will be returned.