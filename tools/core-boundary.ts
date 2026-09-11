import { isAbsolute, relative, resolve, dirname } from 'node:path';
import ts from 'typescript';

/** AST inspection catches imports, re-exports, type imports, require and dynamic import. */
export function coreBoundaryViolations(source: string, filename: string, coreRoot: string): string[] {
  const file = ts.createSourceFile(filename, source, ts.ScriptTarget.Latest, true);
  const errors: string[] = [];
  const check = (specifier: string) => {
    const target = resolve(dirname(filename), specifier);
    const rel = relative(coreRoot, target);
    if (!specifier.startsWith('.') || rel === '..' || rel.startsWith('../') || rel.startsWith('..\\') || isAbsolute(rel)) {
      errors.push(`External core dependency: ${specifier}`);
    }
  };
  for (const ref of file.referencedFiles) check(ref.fileName);
  for (const ref of file.typeReferenceDirectives) errors.push(`External core type dependency: ${ref.fileName}`);
  function visit(node: ts.Node): void {
    if ((ts.isImportDeclaration(node) || ts.isExportDeclaration(node)) && node.moduleSpecifier && ts.isStringLiteral(node.moduleSpecifier)) check(node.moduleSpecifier.text);
    if (ts.isImportTypeNode(node) && ts.isLiteralTypeNode(node.argument) && ts.isStringLiteral(node.argument.literal)) check(node.argument.literal.text);
    if (ts.isExternalModuleReference(node) && node.expression && ts.isStringLiteral(node.expression)) check(node.expression.text);
    if (ts.isCallExpression(node) && (node.expression.kind === ts.SyntaxKind.ImportKeyword || (ts.isIdentifier(node.expression) && node.expression.text === 'require'))) {
      const argument = node.arguments[0];
      if (argument && ts.isStringLiteral(argument)) check(argument.text);
      else errors.push('Non-literal core import');
    }
    if (ts.isIdentifier(node) && ['Date', 'performance', 'fetch', 'XMLHttpRequest', 'WebSocket', 'setTimeout', 'setInterval'].includes(node.text)) errors.push(`Forbidden core runtime: ${node.text}`);
    if (ts.isPropertyAccessExpression(node) && ts.isIdentifier(node.expression) && node.expression.text === 'Math' && node.name.text === 'random') errors.push('Unseeded Math.random');
    ts.forEachChild(node, visit);
  }
  visit(file);
  return errors;
}
