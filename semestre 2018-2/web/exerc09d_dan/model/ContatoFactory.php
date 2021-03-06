<?php

require_once("Contato.php");
require_once("AbstractFactory.php");

/*
* Material utilizado para as aulas práticas da disciplinas da Faculdade de
* Computação da Universidade Federal de Mato Grosso do Sul (FACOM / UFMS).
* Seu uso é permitido para fins apenas acadêmicos, todavia mantendo a
* referência de autoria.
*
*
*
* Classe ContatoFactory - fábrica de contatos.
*
* @author Jane Eleutério
*
* @version 2.2 - 09/Out/2017
*/

class ContatoFactory extends AbstractFactory {


    public function __construct() {

        parent::__construct();
    }

    /**
    * Persiste objetos Contato no banco de dados.
    * @param Contato $obj - Objeto Contato a ser persistido.
    * @return boolean - se conseguiu salvar ou não.
    */
    public function salvar($obj) {

        $contato = $obj;

        try {
            $sql = "INSERT INTO tbcontato (nome, email) VALUES ('"
                . $contato->getNome() . "', '"
                . $contato->getEmail() . "' )";

                if ($this->db->exec($sql)) {
                    $result = true;
                } else {
                    $result = false;
                }
        } catch (PDOException $exc) {
            echo $exc->getMessage();
            $result = false;
        }

        return $result;
    }

    /**
    * Lista os objetos persistidos no banco.
    * @return array -  Array de objetos da classe.
    */
    public function listar(): array {



        $sql = "SELECT * FROM tbcontato";

        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");

            return $resultObject;

        } catch (Exception $exc) {
            echo $exc->getMessage();
            $resultObject = array();
        }

    }

    /**
    * Lista os objetos persistidos no banco, que possuem o $email.
    * @param string $email - email a ser buscado.
    * @return  array -  Array de objetos da classe, ou null se não encontrar
    * objetos.
    */
    public function buscar(string $param): array {
        $sql = "SELECT * FROM tbcontato Where email='" . $param . "'";

        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");
        } catch (Exception $exc) {

            echo $exc->getMessage();
            $resultObject = null;
        }

        return $resultObject;
    }

    public function editar(string $id, string $nome, string $email): bool {
        $sql = "UPDATE tbcontato SET email='" . $email . "', nome='" . $nome . "' WHERE id='" . $id . "'";
        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");
        } catch (Exception $exc) {

            echo $exc->getMessage();
            $resultObject = null;
        }

        return true;
    }


    public function ultimoId(): array {
        $sql = "SELECT * FROM tbcontato ORDER BY id DESC LIMIT 1;";
        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");
        } catch (Exception $exc) {

            echo $exc->getMessage();
            $resultObject = null;
        }

        return $resultObject;
    }

    public function buscarPorId(string $param): array {
        $sql = "SELECT * FROM tbcontato Where id='" . $param . "'";
        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");
        } catch (Exception $exc) {

            echo $exc->getMessage();
            $resultObject = null;
        }

        return $resultObject;
    }

    public function excluir(string $param): bool {
        $sql = "DELETE FROM tbcontato WHERE id='" . $param . "'";

        try {
            $resultRows = $this->db->query($sql);

            if (!($resultRows instanceof PDOStatement)) {
                throw new Exception("Tem erro no seu SQL!<br> '" . $sql . "'");
            }

            $resultObject = $this->queryRowsToListOfObjects($resultRows, "Contato");
        } catch (Exception $exc) {

            echo $exc->getMessage();
            $resultObject = null;
        }

        return true;
    }
}
?>
