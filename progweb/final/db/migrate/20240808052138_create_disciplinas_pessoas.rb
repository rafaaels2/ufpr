class CreateDisciplinasPessoas < ActiveRecord::Migration[7.1]
  def change
    create_table :disciplinas_pessoas do |t|
      t.references :disciplina, null: false, foreign_key: true
      t.references :pessoa, null: false, foreign_key: true

      t.timestamps
    end
    add_index :pessoas_disciplinas, [:pessoa_id, :disciplina_id], unique: true    
  end
end
